<?php
include_once($relPath.'project_states.inc');
include_once($relPath.'filter_project_list.inc');
include_once($relPath.'SettingsClass.inc');
include_once($relPath.'special_colors.inc');
include_once($relPath.'forum_interface.inc'); // get_forum_email_address get_url_to_compose_message_to_user
include_once($relPath.'user_is.inc'); // user_is_a_sitemanager
include_once($relPath.'js_newwin.inc'); // prep_for_links_to_project_pages get_onclick_attr_for_link_to_project_page

// -----------------------------------------------------------------------------

function show_projects_for_round( $round, $show_filter_block, $allow_special_colors_legend )
{
    $initial_project_selector = "state = '{$round->project_available_state}'";

    $filter_custom_display_fields = NULL;

    $columns = array(
        "nameofwork"        => array( 'Title',  _("Title") ),
        "authorsname"       => array( 'Author', _("Author") ),
        "language"          => array( 'Lang',   _("Language") ),
        "genre"             => array( 'Genre',  _("Genre") ),
        "username"          => array( 'PM',     _("Project Manager") ),
        "n_available_pages" => array( 'PgAv',   _("Available Pages") ),
        "n_pages"           => array( 'PgTot',  _("Total Pages") ),
        "days_avail"        => array( 'Days',   _("Days") ),
    );

    show_projects_for_stage(
        $round,
        $initial_project_selector,
        $show_filter_block,
        $filter_custom_display_fields,
        $allow_special_colors_legend,
        $columns );
}

// -----------------------------------------------------------------------------

function show_projects_for_smooth_reading()
{
    global $pguser;

    // Selector for checked out projects
    $initial_project_selector = "
        state = 'proj_post_first_checked_out' AND
        smoothread_deadline > UNIX_TIMESTAMP()
    ";

    $filter_custom_display_fields = array("checkedoutby" => TRUE);

    $columns = array(
        "nameofwork"   => array( 'Title',  _("Title") ),
        "authorsname"  => array( 'Author', _("Author") ),
        "language"     => array( 'Lang',   _("Language") ),
        "genre"        => array( 'Genre',  _("Genre") ),
        "username"     => array( 'PM',     _("Project Manager") ),
        "checkedoutby" => array( 'PP',     _("Post Processor") ),
        "uploaded"     => array(  NULL,    _("Uploaded") ),
        "n_pages"      => array( 'PgTot',  _("Total Pages") ),
        "days_left"    => array( 'Days',   _("Days Left") ),
    );

    // But only show PM, PP, and Uploaded if they are logged in.
    if ( !isset($pguser) )
    {
        unset($columns['username']);
        unset($columns['checkedoutby']);
        unset($columns['uploaded']);

        $filter_custom_display_fields['username']     = FALSE;
        $filter_custom_display_fields['checkedoutby'] = FALSE;
        $filter_custom_display_fields['uploaded']     = FALSE;
    }

    // Show the committed project listing if the user is logged in.
    // This listing should not have the legend display and show
    // neither the filter form or be filtered in any way.
    if ( isset($pguser) )
    {
        $title = _('Projects you have volunteered to Smooth Read');
        echo "\n<h2 align='center'>$title</h2>";

        $stage = get_Stage_for_id("SR");
        $ext_sort_param_name = "order{$stage->id}";
        $ext_sort_setting_name = "{$stage->id}_order";
        $default_ext_sort = 'DaysA';
        $optional_join_clause = "
            INNER JOIN smoothread ON (
                projects.projectid = smoothread.projectid AND
                smoothread.user = '$pguser' AND smoothread.committed <> 0
            )
        ";

        show_project_listing(
            $initial_project_selector,
            $columns,
            $ext_sort_param_name,
            $ext_sort_setting_name,
            $default_ext_sort,
            $stage->id,
            $stage,
            $optional_join_clause
        );
    }

    // Do it again for non-committed (projects available for SR) listing.
    // This listing should have both the filter and legend display.
    if ( isset($pguser) )
    {
        $initial_project_selector .= " AND
            projectid NOT IN (
                SELECT projectid FROM smoothread WHERE smoothread.user = '$pguser'
            )
         ";
    }

    $title = _('Projects Available for Smooth Reading');
    echo "\n<h2 align='center'>$title</h2>";

    // Only show the filters if the user is logged in.
    // Filters don't work correctly unless there's a $pguser session,
    // since the filter terms are stored into the user prefs when
    // modified by a POST request, then read back in by the code
    // displaying and applying the current filter terms and this
    // requires a logged in user account.
    $show_filter_block = isset($pguser);

    show_projects_for_stage(
        get_Stage_for_id("SR"),
        $initial_project_selector,
        $show_filter_block,
        $filter_custom_display_fields,
        /* $allow_special_colors_legend */ TRUE,
        $columns
    );
}

// -----------------------------------------------------------------------------

function show_projects_for_stage(
    $stage,
    $initial_project_selector,
    $show_filter_block,
    $filter_custom_display_fields,
    $allow_special_colors_legend,
    $columns,
    $optional_join_clause="")
// Generate a chunk of HTML consisting of:
// (1) an optional filter widget,
// (2) an optional "Special Days" color legend, and
// (3) a listing of projects.
// 
// Arguments:
// $stage:
//     An instance of the Stage class (or a subclass thereof).
//     This is used for various things, but most importantly,
//     $stage->id is deemed to identify this listing.
// $initial_project_selector:
//     A string containing an SQL condition (suitable for use in a
//     WHERE clause on the 'projects' table) that selects all the
//     projects that would be listed in the absence of a user's filter.
// $show_filter_block:
//     A boolean: should we show the filter widget?
// $filter_custom_display_fields:
//     An array naming any custom fields that should appear in the filter widget.
// $allow_special_colors_legend:
//     A boolean: should we show the special colors legend?
//     (This can be overriden by the corresponding user preference.)
// $columns:
//     An array specifying which columns should appear in the listing.
{
    global $pguser;

    if ($show_filter_block)
    {
        echo "<hr width='75%'>\n";
        process_and_display_project_filter_form($pguser, $stage->id, $stage->name, $_POST, $initial_project_selector, $filter_custom_display_fields);
    }
    $projects_filter = get_project_filter_sql($pguser, $stage->id);
    $filtered_project_selector = "$initial_project_selector $projects_filter";

    // special colors legend
    if ($allow_special_colors_legend )
    {
        // Show the special colors legend if
        // the requestor is a guest (not logged in),
        // or is logged in and hasn't opted to suppress it
        // via the preference "Show Special Colors: No".
        $userSettings =& Settings::get_Settings($pguser);
        if (!isset($pguser) OR !$userSettings->get_boolean('hide_special_colors'))
        {
            echo "<hr width='75%'>\n";
            echo_special_legend($initial_project_selector);
        }
    }

    $ext_sort_param_name = "order{$stage->id}";
    $ext_sort_setting_name = "{$stage->id}_order";
    $default_ext_sort = 'DaysA';

    $anchor = $stage->id;
    echo "\n<a name='$anchor'></a>";

    $title = _('Projects Currently Available');
    echo "\n<h3 align='center'>$title</h3>";
    echo "\n<p>{$stage->description}</p>";

    show_project_listing(
        $filtered_project_selector,
        $columns,
        $ext_sort_param_name,
        $ext_sort_setting_name,
        $default_ext_sort,
        $anchor,
        $stage,
        $optional_join_clause
    );
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function show_projects_for_pool( $pool, $checkedout_or_available )
{
    global $pguser;

    $anchor = $checkedout_or_available;

    if ( $checkedout_or_available == 'checkedout' )
    {
        $header = _('Books I Have Checked Out');

        echo "<h2 align='center'>$header</h2>";

        $initial_project_selector = "state = '{$pool->project_checkedout_state}'";

        // The project must be checked-out to somebody.
        // We're only interested if it's checked out to the current user.
        $initial_project_selector .= " AND checkedoutby = '$pguser'";

        $projects_filter = "";

        echo "<a name='$anchor'></a>\n";
    }
    elseif ( $checkedout_or_available == 'available' )
    {
        $header = _('Books Available for Checkout');

        echo "<h2 align='center'>$header</h2>";

        $available_filtertype_stem = "{$pool->id}_av";

        $initial_project_selector = "state = '{$pool->project_available_state}'";
        process_and_display_project_filter_form($pguser, $available_filtertype_stem, $pool->name, $_POST, $initial_project_selector);
        $projects_filter = get_project_filter_sql($pguser, $available_filtertype_stem);

        echo "<a name='$anchor'></a>\n";
        echo "<center><b>$header</b></center>";
    }
    else
    {
        assert(FALSE);
    }

    $filtered_project_selector = "$initial_project_selector $projects_filter";

    $columns = array(
        "nameofwork"          => array( 'Title',  _("Title") ),
        "authorsname"         => array( 'Author', _("Author") ),
        "language"            => array( 'Lang',   _("Language") ),
        "genre"               => array( 'Genre',  _("Genre") ),
        "n_pages"             => array( 'PgTot',  _("Pages") ),
        $pool->foo_field_name => array( 'Person', $pool->foo_Header ),
        "days_avail"          => array( 'Days',   _("Days") ),
    );

    $ch_or_av = substr( $checkedout_or_available, 0, 2 );
    $ext_sort_param_name = "order_{$checkedout_or_available}";
    $ext_sort_setting_name = "{$pool->id}_{$ch_or_av}_order";
    $default_ext_sort = 'DaysD';

    show_project_listing(
        $filtered_project_selector,
        $columns,
        $ext_sort_param_name,
        $ext_sort_setting_name,
        $default_ext_sort,
        $anchor,
        $pool
    );
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// Create the sortable project listing.
function show_project_listing(

    $filtered_project_selector,
        // an SQL condition (on the 'projects' table) specifying
        // which projects to list.

    $columns,
        // the columns to include in the table, this is an associative
        // array with the key being the column name and the value
        // being an array consisting of:
        //     - the column's name for the purposes of ext_sort
        //     - the column's header-label.

    $ext_sort_param_name,
        // The parameter-name to use when specifying a sort order
        // in a URL query-string.
    $ext_sort_setting_name,
        // the setting name to use when saving the sort order in the db
    $default_ext_sort,
        // The sort to use when one isn't specified by url-parameter or db-setting.

    $anchor,
        // the anchor on the page to jump to

    $stage,
        // stage object (Round, Pool, or Stage)

    $optional_join_clause=""
        // If needed. Default to "", but useful/necessary for SR stage
)
{
    global $code_url, $pguser;

    // Some columns (notably those with numbers in them)
    // have special formatting styles -- specify them here.
    $column_styles = array(
        "n_available_pages" => "text-align: right;",
        "n_pages"   => "text-align: right;",
        "days_avail"=> "text-align: right;",
        "days_left" => "text-align: right;"
    );

    // Load the sort column and direction
    list($curr_sql_sort_col, $curr_sql_sort_dir) =
        process_sorting_control($columns, $ext_sort_param_name, $ext_sort_setting_name, $default_ext_sort);

    $curr_sql_orderclause = "$curr_sql_sort_col $curr_sql_sort_dir";

    // Sort on nameofwork ASC as a secondary criteria if it isn't the primary
    // criteria.
    if($curr_sql_sort_col != "nameofwork")
    {
        $curr_sql_orderclause .= ", nameofwork ASC";
    }

    // The originating request may have query-string settings (other than
    // for $ext_sort_param_name). We should preserve those, and just append the
    // setting for $ext_sort_param_name.
    $other_settings = '';
    foreach ( $_GET as $name => $value )
    {
        if ( $name != $ext_sort_param_name )
        {
            $other_settings .= urlencode($name) . "=" . urlencode($value) . "&amp;";
        }
    }

    // $presort allows us to place certain projects at the top of all project
    // listings such as BEGIN projects, but only for Rounds.
    $presort = "";
    if( is_a( $stage, 'Round' ) )
    {
        $presort = round_project_listing_presort($stage);
        if (!empty($presort)) $presort .= ',';
    }

    // Build and execute the query against the projects table.
    $query = "
        SELECT *,
            (unix_timestamp()    - modifieddate    )/(24 * 60 * 60) AS days_avail,
            (smoothread_deadline - unix_timestamp())/(24 * 60 * 60) AS days_left
        FROM projects
        $optional_join_clause
        WHERE
            $filtered_project_selector
        ORDER BY
            $presort
            $curr_sql_orderclause
    ";
    $result = mysql_query($query);

    if( is_a( $stage, 'Round' ) )
    {
        prep_for_links_to_project_pages();
    }

    // Start the table.
    echo "\n<table class='availprojectlisting'>";
    echo "<tr align=center bgcolor='{$stage->listing_bgcolors[1]}'>";

    // Print out the header row with links.
    foreach($columns as $col_id => $col_info)
    {
        list($ext_sort_col, $label) = $col_info;

        $style_attribute="";
        if(@$column_styles[$col_id])
            $style_attribute="style='{$column_styles[$col_id]}'";

        echo "\n<th $style_attribute>";

        // Change spaces in label to <br>s to force them to wrap. This
        // improves table layout in all browsers.
        $label = preg_replace("/ /","<br>",$label);

        if(is_null($ext_sort_col))
        {
            // We can't sort on this column, so it's a plain header.
            echo $label;
        }
        else
        {
            if($col_id == $curr_sql_sort_col)
            {
                // The header-link reverses the current sort direction.
                $ext_sort_dir = ($curr_sql_sort_dir == "asc" ? "D" : "A");
            }
            else
            {
                // The header-link does an ascending sort.
                $ext_sort_dir = 'A';
            }
            $ext_sort = $ext_sort_col . $ext_sort_dir;

            $sort_url = "?{$other_settings}{$ext_sort_param_name}={$ext_sort}#{$anchor}";
            echo "<a href='$sort_url'>$label</a>";

            if($col_id == $curr_sql_sort_col)
            {
                // Add a visual indicator that this is the sorted-by column,
                // and show which way the column has been sorted.
                $img_url = "$code_url/graphics/sort_{$curr_sql_sort_dir}.png";
                echo "&nbsp;<img src='$img_url'>";
            }
        }

        echo "</th>";
    }

    echo "</tr>";

    if(isset($pguser))
    {
        // Determine whether to use special colors or not
        // (this does not affect the alternating between two
        // background colors) in the project listing.
        // Regardless of the preference, don't display
        // special colors to newbies.
        $userSettings =& Settings::get_Settings($pguser);
        $show_special_colors = (get_pages_proofed_maybe_simulated() >= 10
                                && !$userSettings->get_boolean('hide_special_colors'));
    }
    else
    {
        // Visitors see the special colours.
        $show_special_colors = TRUE;
    }

    // Show email addresses for site administrators
    $show_email = user_is_a_sitemanager();

    $rownum = 0;

    while ($book=mysql_fetch_assoc($result))
    {
        $bgcolor = $stage->listing_bgcolors[$rownum % 2];

        // Special colours for special books of various types
        if ($show_special_colors)
        {
            $special_color = get_special_color_for_project($book);
            if (!is_null($special_color))
            {
                $bgcolor = $special_color;
            }
        }

        echo "<tr bgcolor='$bgcolor'>";

        foreach($columns as $col_id => $col_info)
        {
            if($col_id == "nameofwork")
            {
                $eURL = "$code_url/project.php?id={$book['projectid']}&amp;expected_state={$book['state']}";
                $onclick_attr = (
                    is_a( $stage, 'Round' )
                    ? get_onclick_attr_for_link_to_project_page($eURL)
                    : ""
                );
                $cell = "<a href=\"$eURL\" $onclick_attr>{$book['nameofwork']}</a>";
            }
            elseif($col_id == "genre")
            {
                $genre = $book['genre'];
                if ($book['difficulty'] == "beginner")
                {
                    if( is_a( $stage, 'Round' ) )
                    {
                        if ( $stage->is_a_mentee_round() )
                            $genre = _("BEGINNERS ONLY")." ".$genre;
                        else if ( $stage->is_a_mentor_round() )
                            $genre = _("MENTORS ONLY")." ".$genre;
                    }
                    elseif ( $stage->id == 'SR' )
                    {
                        $genre = _("BEGINNERS")." ".$genre;
                    }
                }
                elseif ($book['difficulty'] == "easy")
                {
                    $genre = _("EASY")." ".$genre;
                }
                elseif ($book['difficulty'] == "hard")
                {
                    $genre = _("HARD")." ".$genre;
                }
                $cell = $genre;
            }
            elseif($col_id == "username" || $col_id == "checkedoutby" || $col_id == "postproofer" || $col_id == "correctedby")
            // Create email or PM links for usernames.
            {
                $user_name = $book[$col_id];
                if ($show_email)
                {
                    $email = get_forum_email_address($user_name);
                    $contact_url = "mailto:$email";
                }

                if (!$show_email || $email == "")
                {
                    $contact_url = get_url_to_compose_message_to_user($user_name);
                }

                $cell = "<a href='$contact_url'>$user_name</a>";
            }
            elseif($col_id == "uploaded")
            // Uploaded is a special case because it is pulled from the
            // filesystem and not from the database, and is hence not sortable.
            {
                global $projects_dir;
                if ($done_files = glob("$projects_dir/{$book['projectid']}/*smooth_done_*.zip") ) {
                   $num_done = count($done_files);
                } else {
                   $num_done = 0;
                }
                $cell = $num_done;
            }
            elseif( $col_id == "days_avail" || $col_id == "days_left" )
            {
                $cell = sprintf( "%.1f", $book[$col_id] );
            }
            else
            {
                $cell = $book[$col_id];
            }

            // Determine any special style formatting.
            $style_attribute="";
            if(@$column_styles[$col_id])
                $style_attribute="style='{$column_styles[$col_id]}'";

            // Finally, output the table cell.
            echo "\n<td $style_attribute>$cell</td>";
        }

        echo "</tr>\n";
        $rownum++;
    }

    echo "</table>\n<br>";

    // Free the search results.
    mysql_free_result($result);
}

// -----------------------------------------------------------------------------

function process_sorting_control($columns, $ext_sort_param_name, $ext_sort_setting_name, $default_ext_sort)
// This function's purpose is to intelligently determine the user's
// intent for a sort criteria. It does this by:
//   1. Checking $_GET for explicit sort order changes
//   2. If that fails pulling the previous sort order from the database
//   3. If that fails using a default sort order (first column) 
// After we have a sort criteria, validate it and fall back to
// the default sort column in ascending order if necessary.
// After validation the sort criteria is written to the database.
// Arguments:
//   $columns      - an associative array containing the valid columns in the
//                   table
//   $ext_sort_param_name - the parameter-name to use in the URL query-string
//   $ext_sort_setting_name - the setting to use in the usersettings table. This value
//                   is also used in determining the sort order via the URL.
//                   Doing so allows for multiple tables to be on the same
//                   page and sorted in different orders.
//   $default_ext_sort - the sort to use when one isn't specified by
//                   url-parameter or db-setting.
// Returns the array($curr_sql_sort_col, $curr_sql_sort_dir):
//   $curr_sql_sort_col - name of the column to sort by
//   $curr_sql_sort_dir - direction to sort by (asc or desc)
{
    global $pguser;

    $local_trace = FALSE; // set to TRUE when debugging

    // Load user settings
    $userSettings =& Settings::get_Settings($pguser);
    // Note that if requestor is a guest, $pguser is NULL, and
    // $userSettings->get_value() will return
    // the supplied default (if any) or NULL (if not).
    // And $userSettings->set_value() is a no-op.

    // We get an ext_sort from one of three places,
    // which we consult in the following order,
    // stopping as soon as we get a valid value.
    $ext_sort_providers = array(

        // A parameter in the invoking URL's query-string:
        array(
            sprintf( _("URL parameter '%s'"), $ext_sort_param_name ),
            @$_GET[$ext_sort_param_name] ),

        // A setting in the usersettings table in the DB (accessed via Setting class):
        array(
            sprintf( _("DB setting '%s'"), $ext_sort_setting_name ),
            $userSettings->get_value($ext_sort_setting_name) ),

        // A fallback default:
        array(
            _("site default"),
            $default_ext_sort ),

        // And if none of those works, give up.
        'ABORT'
    );

    foreach ( $ext_sort_providers as $ext_sort_provider )
    {
        if ($ext_sort_provider == 'ABORT')
        {
            echo "\n<p>";
            echo _("system error: the fallback default failed!");
            echo "</p>";
            exit;
        }

        list($provider_noun, $curr_ext_sort) = $ext_sort_provider;
        if ($local_trace)
        {
            echo "\n<p>";
            echo sprintf(
                    _("%s supplies value '%s'"),
                    $provider_noun, $curr_ext_sort );
            echo "</p>";
        }

        if ( is_null($curr_ext_sort) )
        {
            // This (potential) provider isn't supplying a value this time.
            continue;
        }

        // Parse the $curr_ext_sort into its column & direction parts.
        $curr_ext_sort_col = substr($curr_ext_sort, 0, -1);
        $curr_ext_sort_dir = substr($curr_ext_sort, -1);

        // Check column & convert to sql...

        // Find the entry in $columns whose $ext_sort_col matches $curr_ext_sort_col.
        foreach ( $columns as $col_id => $col_info )
        {
            list($ext_sort_col, $label) = $col_info;
            if ($ext_sort_col == $curr_ext_sort_col)
            {
                $curr_sql_sort_col = $col_id;
                break;
            }
        }

        if ( !isset($curr_sql_sort_col) )
        {
            echo "\n<p class='error'>\n";
            echo sprintf(
                    _("%s value '%s' has bad column part: '%s'"),
                    $provider_noun, $curr_ext_sort, $curr_ext_sort_col );
            echo "\n</p>";
            continue;
        }

        // Check direction & convert to sql...
        if ( $curr_ext_sort_dir == 'A' )
        {
            $curr_sql_sort_dir = 'asc';
        }
        elseif ( $curr_ext_sort_dir == 'D' )
        {
            $curr_sql_sort_dir = 'desc';
        }
        else
        {
            echo "\n<p class='error'>\n";
            echo sprintf(
                    _("%s value '%s' has bad direction part: '%s'"),
                    $provider_noun, $curr_ext_sort, $curr_ext_sort_dir );

            // We could skip to the next provider via 'continue',
            // but that seems a bit draconian.

            $curr_ext_sort_dir = 'A';
            $curr_sql_sort_dir = 'asc';
            $curr_ext_sort = $curr_ext_sort_col .  $curr_ext_sort_dir;

            echo "\n";
            echo sprintf(
                    _("Using '%s' instead."),
                    $curr_ext_sort_dir );
            echo "\n</p>";
        }

        // This provider supplied a valid sort value, so look no further.
        break;
    }

    // Save the results to the database.
    {
        // Get the saved sort order.
        // (Or if there isn't one, use the default,
        // which avoids unnecessarily saving the default.)
        $saved_ext_sort = $userSettings->get_value($ext_sort_setting_name, $default_ext_sort);

        // If sort order has changed, save it to database.
        if ($curr_ext_sort != $saved_ext_sort)
        {
            if ($local_trace)
            {
                echo "\n<p>";
                echo sprintf(
                        _("DB setting '%s' receives value '%s'"),
                        $ext_sort_setting_name, $curr_ext_sort );
                echo "</p>";
            }
            $userSettings->set_value($ext_sort_setting_name, $curr_ext_sort);
        }
    }

    // Make some sanity-check assertions on the values we're about to return.

    // $curr_sql_sort_col is the col_id of one of the columns in $columns:
    assert( isset($columns[$curr_sql_sort_col]) );
    // And that column is sortable:
    assert( !is_null($columns[$curr_sql_sort_col][0]) );

    assert( $curr_sql_sort_dir == "asc" || $curr_sql_sort_dir == "desc" );

    return array($curr_sql_sort_col, $curr_sql_sort_dir);
}

// vim: sw=4 ts=4 expandtab