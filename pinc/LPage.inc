<?php

include_once($relPath.'project_states.inc');
include_once($relPath.'project_trans.inc');
include_once($relPath.'site_vars.php');
include_once($relPath.'stages.inc');
include_once($relPath.'user_is.inc');
include_once($relPath.'DPage.inc');
include_once($relPath.'Project.inc');
include_once($relPath.'page_tally.inc');
include_once($relPath.'project_continuity.inc');
include_once($relPath.'abort.inc');
include_once($relPath.'maybe_mail.inc');

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function get_available_page( $projectid, $proj_state, $pguser, &$err )
// Returns an LPage, unless no page is available,
// in which case returns NULL and sets $err.
{
    global $preceding_proofer_restriction;
    // Make sure project is still in same state.
    $err = project_continuity_test($projectid, $proj_state, TRUE);
    if ( $err ) return NULL;

    $round = get_Round_for_project_state($proj_state);

    // Normally, pages are served in order of "page number"
    // (i.e., by the 'image' field)
    $order = 'image ASC';
    // but this can be overridden below.

    $project = new Project($projectid);

    $err = can_user_get_pages_in_project( $pguser, $project, $round );
    if ( $err ) return NULL;

    if ( $project->difficulty == 'beginner' and $round->is_a_mentor_round() )
    {
            // For beginner projects in a mentor round,
            // we serve up pages in a different order,
            // to facilitate mentoring:
            // 1) Mentors like to give prompt feedback.
            // 2) Mentors like to deal with all of one proofreader's pages at a time.
            //
            // We look at the save-times recorded in the mentee round for pages
            // in this project, and for each mentee, ascertain their earliest
            // such save-time, and then order the mentees according to that time.
            // (So the mentee with the "longest-waiting" page comes first.
            // In the unlikely event that two mentees have exactly the same
            // earliest-save-time (down to the second), we break the tie by
            // taking the users in alphabetical order by username.)
            //
            // Taking the mentees in the above order, we serve each one's pages
            // in 'book order' (i.e., ordered by 'image' field).
            //
            // (See http://www.pgdp.net/phpBB3/viewtopic.php?p=1027908#p1027908 and following.)
            $order = "
                (
                    SELECT MIN({$round->mentee_round->time_column_name})
                    FROM $projectid
                    WHERE {$round->mentee_round->user_column_name}
                      = p.{$round->mentee_round->user_column_name}
                ),
                {$round->mentee_round->user_column_name},
                image
            ";
            // Note that this uses a subquery (in an ORDER clause),
            // which I believe requires at least MySQL 4.1.
    }

    // The page to be retrieved must be an available page, of course.
    $restrictions = "state='{$round->page_avail_state}'";

    // Are there any other restrictions that the page must satisfy?
    // (This should maybe be a property of $round.)
    if ( empty($preceding_proofer_restriction) )
    {
        // Nope, no other restrictions.
    }
    else if ($preceding_proofer_restriction == 'not_immediately_preceding')
    {
        // Don't give this user a page that they worked on in the preceding round.
        // (Where "preceding" takes into account skipped rounds.)
        if ( $round->round_number > 1 )
        {
            // We need an SQL expression for the preceding proofreader.
            $preceding_proofer = "CASE";
            for ( $rn = $round->round_number-1; $rn > 0; $rn-- )
            {
                $earlier_round = get_Round_for_round_number( $rn );
                $ucn = $earlier_round->user_column_name;
                $preceding_proofer .= " WHEN LENGTH($ucn) THEN $ucn";
            }
            // What if all of the earlier rounds were skipped?
            // (It's pretty unlikely, but we should allow for it.)
            // All of the WHEN LENGTH(...) clauses will fail.
            //
            // If the CASE expr doesn't have an ELSE clause, it will yield NULL,
            // so we'll end up requiring that
            //     NULL != '$pguser'
            // which is always NULL (effectively false), so no pages will
            // satisfy the restriction, which is not what we want.
            //
            // Instead, add an ELSE clause saying that the preceding proofreader's
            // name was the empty string. So we'll end up requiring that
            //     '' != '$pguser'
            // which is always true, so any (available) page will satisfy the
            // restriction, which is what we want.
            $preceding_proofer .= " ELSE ''";
            $preceding_proofer .= " END";

            $restrictions .= " AND $preceding_proofer != '$pguser'";
        }
    }
    else if ($preceding_proofer_restriction == 'not_previously_proofed')
    {
        // Don't give this user a page that they have worked on before.
        for ( $rn = 1; $rn < $round->round_number; $rn++ )
        {
            $earlier_round = get_Round_for_round_number( $rn );
            $restrictions .= " AND {$earlier_round->user_column_name} != '$pguser'";
        }
    }
    else
    {
        // The empty/null (i.e., no restriction) case has already been dealt with.
        die("Bad value for 'preceding_proofer_restriction': '$preceding_proofer_restriction'.");
    }

    // Find page to be proofread.
    // (It appears that a simultaneous query from someone else would return the same page.)
    $dbQuery = "
        SELECT image, state
        FROM $projectid AS p
        WHERE $restrictions
        ORDER BY $order
        LIMIT 1
    ";
    $result = mysql_query($dbQuery);
    if ($result === FALSE) die(mysql_error());
    $numrows = mysql_num_rows($result);
    if ($numrows == 0)
    {
        $err = _("No more files available for proofreading for this round of the project.");
        return NULL;
    }
    $npage = mysql_fetch_assoc($result);

    $imagefile = $npage['image'];

    $lpage = new LPage( $projectid, $imagefile, $npage['state'], 0 );

    $lpage->checkout( $pguser );

    $err = NULL;
    return $lpage;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function get_indicated_LPage(
    $projectid,
    $proj_state,
    $imagefile,
    $page_state,
    $reverting_to_orig )
// Returns an LPage, unless the parameters do not correctly identify a page,
// in which case it aborts.
{
    // Make sure project is still in same state.
    $err = project_continuity_test($projectid, $proj_state, FALSE);
    if ( $err ) abort($err);

    $res = mysql_query("
        SELECT state
        FROM $projectid
        WHERE image='$imagefile'
    ") or die(mysql_error());
    if (mysql_num_rows($res) == 0)
    {
        // This should only happen if they've been tweaking URLs.
        // Or if the page has actually been deleted.
        $err = _('No such page.');
        abort($err);
    }

    list($current_page_state) = mysql_fetch_row($res);

    if ($page_state != $current_page_state)
    {
        $err = sprintf(
            _('Page %1$s has changed state from %2$s to %3$s, so your request is invalid.'),
            $imagefile, $page_state, $current_page_state );
        abort($err);
    }

    return new LPage($projectid, $imagefile, $page_state, $reverting_to_orig);
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

class LPage
// The "L" stands for "Logic",
// as in the 3 tiers of Data, Logic, and Presentation.
// (For pages, the Data layer is pinc/DPage.inc,
// and the Presentation layer is most of tools/proofers/*.
// However, we're not very strict about the divisions yet.)
{
    function LPage($projectid, $imagefile, $page_state, $reverting_to_orig)
    {
        $this->projectid  = $projectid;
        $this->imagefile  = $imagefile;
        $this->page_state = $page_state;
        $this->reverting_to_orig = $reverting_to_orig;

        // It's debatable whether LPage should have a 'reverting_to_orig' property.
        // You could argue that it's an interface-level artifact,
        // and thus belongs in PPage.

        $this->round = get_Round_for_page_state($page_state);
    }

    // -------------------------------------------------------------------------

    function can_be_reverted_to_last_save()
    {
        return ( $this->reverting_to_orig==1
            || $this->page_state == $this->round->page_save_state
            || $this->page_state == $this->round->page_temp_state
        );
    }

    function get_text()
    {
        if ( $this->reverting_to_orig==1
            || $this->page_state == $this->round->page_avail_state
            || $this->page_state == $this->round->page_out_state )
        {
            $desired_column_name = $this->round->prevtext_column_name;
        }
        else
        {
            $desired_column_name = $this->round->text_column_name;
        }

        $q = "
            SELECT $desired_column_name
            FROM {$this->projectid}
            WHERE image='{$this->imagefile}'
        ";
        $res = mysql_query($q);
        list($page_text) = mysql_fetch_row($res);

        return $page_text;
    }

    function get_language()
    {
        $project = new Project($this->projectid);
        $lang = proj_lang_code($project->language,"primary");
        return $lang;
    }

    function get_username_for_round( $round )
    {
        $res = mysql_query("
            SELECT {$round->user_column_name}
            FROM {$this->projectid}
            WHERE image = '{$this->imagefile}'
        ");
        $row = mysql_fetch_assoc($res);
        return $row[$round->user_column_name];
    }

    // -------------------------------------------------------------------------

    function checkout( $user )
    {
        $this->page_state =
            Page_checkout( $this->projectid, $this->imagefile, $this->round, $user );
    }

    function saveAsInProgress( $page_text, $user )
    {
        $this->page_state = Page_saveAsInProgress( $this->projectid, $this->imagefile, $this->round, $user, $page_text );
        $this->reverting_to_orig = 0;
    }

    function saveAsDone($text_data,$pguser)
    {
        $this->page_state =
            Page_saveAsDone( $this->projectid, $this->imagefile, $this->round, $pguser, $text_data );

        // add to user page count
        page_tallies_add( $this->round->id, $pguser, +1 );
    }

    function returnToRound($pguser)
    {
        $this->page_state =
            Page_returnToRound( $this->projectid, $this->imagefile, $this->round, $pguser );
    }

    function revertToOriginal()
    {
        $this->reverting_to_orig = 1;
    }

    function revertToSaved()
    {
        $this->page_state = $this->round->page_temp_state;
        $this->reverting_to_orig = 0;
    }

    function markAsBad( $user, $reason )
    // Return TRUE if this report causes project to be marked bad.
    {
        global $code_url, $site_abbreviation, $PAGE_BADNESS_REASONS;

        Page_markAsBad( $this->projectid, $this->imagefile, $this->round, $user, $reason );

        // Advise PM that the page has been marked bad.
		$body_blurb =
"Page {$this->imagefile} of this project has been marked bad due to {$PAGE_BADNESS_REASONS[$reason]}.
Please visit
    $code_url/tools/project_manager/handle_bad_page.php?projectid={$this->projectid}&image={$this->imagefile}
to make any needed changes and make the page available for proofreading again.
Until this report has been resolved, the project will not be able to leave
the current round. If 10 pages are marked bad by at least 3 different users,
the project will automatically be made unavailable.";
        maybe_mail_project_manager($this->projectid, $body_blurb, "$site_abbreviation: Bad Page");

        // Now determine whether the project as a whole should be marked bad.
        // We need 10 or more bad pages reported by 3 or more distinct users.
        // (Note duplication of code with pages_indicate_bad_project() in automodify.php)

        $n_bad_pages =
            Project_getNumPagesInState(
                $this->projectid, $this->round->page_bad_state );
        if ( $n_bad_pages < 10 ) return FALSE;

        $n_distinct_reporters =
            Project_getNumPagesInState(
                $this->projectid, $this->round->page_bad_state, "DISTINCT(b_user)" );
        if ($n_distinct_reporters < 3) return FALSE;

        $error_msg = project_transition( $this->projectid, $this->round->project_bad_state, PT_AUTO );
        if ($error_msg)
        {
            echo "$error_msg<br>\n";
        }
        return TRUE;
    }

    // -----------------------------------------------------------------------------

    function resume_saved_page( $pguser )
    {
        if ($this->page_state == $this->round->page_save_state)
        {
            // Page comes from DONE.

            // no need to update text, just record state change
            $this->page_state = Page_reopen( $this->projectid, $this->imagefile, $this->round, $pguser );

            // When it was saved, the user's page-count was incremented.
            // Now they are 'unsaving' it, so decrement their page-count.
            // They'll get it back if/when they save-as-done again.
            // (Plugs a former page-count cheat.)

            page_tallies_add( $this->round->id, $pguser, -1 );
        }
        else
        {
            // Page comes from IN PROGRESS.

            // Resuming such a page has no actual effect on the page,
            // though it might in the future.

            // For now, just confirm that the user is entitled to resume
            // this page. (In the DONE case, Page_reopen performs this check.)

            $res = mysql_query("
                SELECT {$this->round->user_column_name}
                FROM $this->projectid
                WHERE image='$this->imagefile'
            ");
            list($current_round_user) = mysql_fetch_row($res);

            if ($pguser != $current_round_user)
            {
                $err = sprintf(
                    _("You (%s) do not have the necessary access to page %s"),
                    $pguser, $this->imagefile );
                abort($err);
            }
        }
    }

} // end class

// vim: sw=4 ts=4 expandtab
?>