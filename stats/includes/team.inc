<?php
//A file full of functions used in team stats

include_once($relPath.'prefs_options.inc'); // PRIVACY_*
include_once($relPath.'site_vars.php');
include_once($relPath.'page_tally.inc');
include_once($relPath.'ThemedTable.inc');
include_once($relPath.'forum_interface.inc');
include_once($relPath.'misc.inc'); // get_enumerated_param(), get_integer_param(), attr_safe(), get_filelist()
include_once($relPath.'../stats/includes/common.inc');

$team_avatars_dir = "$dyn_dir/teams/avatar";
$team_avatars_url = "$dyn_url/teams/avatar";

$team_icons_dir   = "$dyn_dir/teams/icon";
$team_icons_url   = "$dyn_url/teams/icon";

function select_from_teams( $where_body, $other_clauses='' )
{
    global $teams_ELR_page_tallyboard;
    list($joined_with_team_ELR_page_tallies,$team_ELR_page_tally_column) =
        $teams_ELR_page_tallyboard->get_sql_joinery_for_current_tallies('id');

    if ($where_body == '') $where_body = '1';

    $q = "
        SELECT
            user_teams.*,
            $team_ELR_page_tally_column AS page_count
        FROM user_teams
            $joined_with_team_ELR_page_tallies
        WHERE $where_body
        $other_clauses
    ";
    $res = mysql_query($q);
    if (!$res) die(mysql_error());
    return $res;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function showTeamInformation($curTeam, $tally_name)
{
    showTeamProfile($curTeam);

    showTeamTallySelector($curTeam, $tally_name );

    if ( !is_null($tally_name) )
    {
        showTeamStats($curTeam, $tally_name);
        showTeamMbrs($curTeam, $tally_name);

        // Only show the team history if they are more than a day old
        $now = time();
        if (($now - $curTeam['created']) > 86400)
        {
            showTeamHistory($curTeam, $tally_name);
        }
    }
}

// -----------------------------------------------------------------------------

function showTeamProfile($curTeam, $preview=FALSE)
{
    global $theme, $userP;
    global $team_avatars_url;
    global $code_url;

    $team_id = $curTeam['id'];

    if($team_id)
        $xml_doc_url = "$code_url/stats/teams/teams_xml.php?id=$team_id";
    else
        $xml_doc_url = "";
    $xml_img_url = "$code_url/graphics/xml.gif";
    $xml_img_tag = "<img src='$xml_img_url' border='0' width='36' height='14' style='vertical-align: middle;'>";
    $xml_button = "<a href='$xml_doc_url'>$xml_img_tag</a>";

    // Allow team owner and site administrators to edit the team
    if (!$preview && (($userP['u_id'] == $curTeam['owner']) || user_is_a_sitemanager()))
    {
        $op_url = "tedit.php?tid=$team_id";
        $text = _('Edit');
        $colored_text = "<font color='".$theme['color_headerbar_font']."' size='2'>$text</font>";
        $editlink = "&nbsp;[<a href='$op_url'>$colored_text</a>]";
    }
    else
    {
        $editlink = '';
    }

    if ($preview || empty($userP))
    {
        // The requestor is not logged in,
        // so they can't join or quit the team.
        $joinquitlink = '';
    }
    else
    {
        if ($userP['team_1'] != $team_id && $userP['team_2'] != $team_id && $userP['team_3'] != $team_id)
        {
            $op = 'jointeam';
            $text = _('Join');
        }
        else
        {
            $op = 'quitteam';
            $text = _('Quit');
        }
        $op_url = "../members/$op.php?tid=$team_id";
        $colored_text = "<font color='".$theme['color_headerbar_font']."' size='2'>$text</font>";
        $joinquitlink = "&nbsp;[<a href='$op_url'>$colored_text</a>]";
    }

    if($curTeam['topic_id'])
        $last_post_date = get_last_post_time_in_topic($curTeam['topic_id']);
    else
        $last_post_date = NULL;
    if ($last_post_date != NULL) {
        $last_post_date = date("n/j/Y g:i:sA", $last_post_date);
    }

    $now = time();
    $daysInExistence = number_format(floor(($now - $curTeam['created'])/86400));

    if(empty($curTeam['avatar'])) { $curTeam['avatar'] = "avatar_default.png"; }

    echo "\n<table cellspacing='0' cellpadding='0' width='95%'>";
    echo "\n<tr>";
    echo "<td align='center' width='30%' bgcolor='" . $theme['color_mainbody_bg']."'>";
    echo "<img border='0' src='$team_avatars_url/";
    echo $curTeam['avatar'] . "' alt='".attr_safe(strip_tags($curTeam['teamname']))."'></td>";
    echo "<td width='70%'>";

    $t = new ThemedTable(
        2,
        $curTeam['teamname'] . "&nbsp;$xml_button$editlink$joinquitlink",
        array( 'width' => 100 )
    );
    $t->set_column_widths( 25, 75 );

    $t->row(
        _("Created"),
        date("m/d/Y", $curTeam['created'])
        . "&nbsp;(" . sprintf(_("%s days"), $daysInExistence) . ")"
    );

    $t->row(
        _("Created by"),
        $curTeam['createdby']
    );

    $owner_url = "$code_url/stats/members/mdetail.php?id=" . $curTeam['owner'];
    $owner_name = get_username_for_uid($curTeam['owner']);
    $t->row(
        _("Leader"),
        "<a href='$owner_url'>" . html_safe($owner_name) . "</a>"
    );

    $t->row(
        _("Description"),
        html_safe($curTeam['team_info'])
    );

    $webpage = $curTeam['webpage'];
    $t->row(
        _("Website"),
        "<a href='" . attr_safe($webpage) . "' target='_new'>" . html_safe($webpage) . "</a>"
    );

    $topic_url = "$code_url/stats/teams/team_topic.php?team=$team_id";
    $t->row(
        _("Forums"),
        "<a href='$topic_url'>"._("Team Discussion")."</a> "
        . ( is_null($last_post_date) ? '' : "("._("Last Post:")." $last_post_date)" )
    );

    if($team_id)
        $mbrCountRank = team_get_member_count_rank($team_id);
    else
        $mbrCountRank = 0;
    $t->row(
        _("Members")
            . " <i>("._("Rank").")</i>",
        number_format($curTeam['member_count'])
            . "&nbsp;<i>(#$mbrCountRank)</i>"
    );
    $t->row(
        _("Current Members"),
        number_format($curTeam['active_members'])
    );
    $t->row(
        _("Retired Members"),
        number_format($curTeam['member_count'] - $curTeam['active_members'])
    );

    $t->end();

    echo "</td></tr></table><p>";
}

function team_get_member_count_rank( $team_id )
{
    $result = mysql_query("
        SELECT id
        FROM user_teams
        ORDER BY member_count DESC
    ");
    $i = 1;
    while ( list($id) = mysql_fetch_row($result) ) {
        if ($id == $team_id) { return $i; }
        $i++;
    }
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function showTeamTallySelector( $curTeam, $tally_name )
{
    global $page_tally_names;

    $team_id = $curTeam['id'];

    $choices = '';
    foreach ( $page_tally_names as $t_name => $t_title )
    {
        if (!empty($choices))
        {
            $choices .= " | ";
        }

        $text = $t_name;
        if ($t_name != $tally_name)
        {
            $text = "<a href='tdetail.php?tid=$team_id&amp;tally_name=$t_name'>$text</a>";
        }
        $choices .= $text;
    }
    echo "\n";
    echo "<center>";
    echo "<p>";
    echo _("Display team stats for different rounds:") . ' ' . $choices;
    echo "</p>";
    echo "</center>";

    echo "\n";
    echo "<hr width='95%'>";

    if ( is_null($tally_name) ) return;

    $tally_title = $page_tally_names[$tally_name];
    echo "\n";
    echo "<center>";
    echo "<p>";
    echo $tally_title;
    echo "</p>";
    echo "</center>";
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function showTeamStats($curTeam, $tally_name) {

    $teams_tallyboard = new TallyBoard( $tally_name, 'T' );

    $vitals = $teams_tallyboard->get_vitals( $curTeam['id'] );

    $bestDayTime = date("M. jS, Y", ($vitals->best_delta_timestamp-1));

    $t = new ThemedTable(
        2,
        _("Team Statistics"),
        array(
            'width' => 95,
            'rollover_highlight' => TRUE
        )
    );
    $t->set_column_widths( 60, 40 );

    $t->row(
        _("Total Pages")
            . " <i>("._("Rank").")</i>",
        "<b>" . number_format($vitals->current_tally) . "</b>"
            . "&nbsp;<i>(#$vitals->current_rank)</i>"
    );

    $avg_pages_per_day = get_daily_average($curTeam['created'],$vitals->current_tally);
    $t->row(
        _("Avg. Pages per Day"),
        "<b>" . number_format($avg_pages_per_day,1) . "</b>"
    );
    $t->row(
        _("Most Pages Done per Day"),
        "<b>" . number_format($vitals->best_delta) . "</b>"
            . "&nbsp;<i>($bestDayTime)</i></b>"
    );

    $t->end();
}

function showTeamMbrs($curTeam, $tally_name) {
    global $code_url, $site_abbreviation;

    $order       = get_enumerated_param($_GET, 'order', 'mbr', array('mbr', 'pages', 'date'));
    $direction   = get_enumerated_param($_GET, 'direction', 'asc', array('asc', 'desc'));
    $req_team_id = get_integer_param($_GET, 'tid', null, 0, null);

    $subtitle = null;
    $latestMbr = mysql_query("SELECT users.username AS Username, users.u_privacy AS Privacy FROM user_teams INNER JOIN users ON user_teams.latestUser = users.u_id WHERE user_teams.id = ".$curTeam['id']." AND user_teams.latestUser <> 0");
    $row = mysql_fetch_assoc($latestMbr);
    if ($row) {
        if ($row["Privacy"] == PRIVACY_PUBLIC || ($row["Privacy"] == PRIVACY_PRIVATE && isset($GLOBALS['pguser']))) {
            $subtitle = sprintf(_("Please welcome <b>%s</b> as the latest member to join this team!"), $row["Username"]);
        }
    }

    $t = new ThemedTable(
        3,
        _("Team Member Details"),
        array(
            'width' => 95,
            'subtitle' => $subtitle,
            'rollover_highlight' => TRUE
        )
    );
    $t->set_column_widths( 35, 35, 30 );

    $header_content_ = array();

    $column_descriptors =
        array(
            array( 'mbr',   'username',      'asc',  _("Username")        ),
            array( 'pages', 'current_tally', 'desc', _("Pages Completed") ),
            array( 'date',  'date_created',  'asc',  sprintf(_("Date Joined %s"),$site_abbreviation)  )
        );
    foreach ( $column_descriptors as $column_descriptor )
    {
        list( $cd_order, $cd_dbcolumn, $cd_default_dir, $cd_header_text ) =
            $column_descriptor;
        if ( $cd_order == $order ) {
            // This is the column by which we are to sort the table.

            // So here's the db-column to ORDER BY in the SQL query.
            $orderby = $cd_dbcolumn;

            // Mark this column-header with a direction-indicator.
            $arrow = ($direction == "asc") ? 'down' : 'up';
            $dir_indicator = " <img src='$code_url/graphics/$arrow.gif'>";
            // (You might think that if the column is sorted in ascending
            // order, we should use an up-arrow. But I'm following the
            // Netscape/Mozilla convention, which is the opposite.)

            // And provide a link to sort by this column in the opposite direction.
            $dir = ($direction == "asc") ? "desc" : "asc";

        } else {
            // This is a non-sorting column (for this request).

            $dir_indicator = '';

            // And provide a link to sort by this column in its default direction.
            $dir = $cd_default_dir;
        }
        $url = "tdetail.php?tid=".$curTeam['id']."&tally_name=$tally_name&order=$cd_order&direction=$dir";

        $header_content_[] = "<a href='$url'>$cd_header_text</a>$dir_indicator";
    }

    $t->column_headers( $header_content_ );

    $users_tallyboard = new TallyBoard( $tally_name, 'U' );

    list( $joined_with_user_tallies, $user_tally_column ) =
        $users_tallyboard->get_sql_joinery_for_current_tallies( 'u_id' );

    $mbrQuery = mysql_query("
        SELECT *, $user_tally_column AS current_tally
        FROM users $joined_with_user_tallies
        WHERE {$req_team_id} IN (team_1, team_2, team_3)
        ORDER BY $orderby $direction
    ");

    $totalAnonUsers = 0;
    $totalPagesAnonUsers = 0;
    while ($curMbr = mysql_fetch_assoc($mbrQuery)) {
        if ($curMbr['u_privacy'] == PRIVACY_ANONYMOUS)
        {
            $totalAnonUsers++;
            $totalPagesAnonUsers += $curMbr['current_tally'];
        }
        elseif ($curMbr['u_privacy'] == PRIVACY_PRIVATE && !isset($GLOBALS['pguser']))
        {
            $totalAnonUsers++;
            $totalPagesAnonUsers += $curMbr['current_tally'];
        }
        else
        {
            $username = $curMbr['username'];
            $member_url = "$code_url/stats/members/mdetail.php?id=" . $curMbr['u_id'];
            $t->row(
                "<a href='$member_url'>$username</a>",
                number_format($curMbr['current_tally']),
                date("m/d/Y", $curMbr['date_created'])
            );
        }
    }
    if ($totalAnonUsers > 0)
    {
        $t->row(
            isset($GLOBALS['pguser']) ? _("Anonymous users") : _("Anonymous/Private users"), 
            number_format($totalPagesAnonUsers),
            "&nbsp;"
        );
    }

    $t->end();
}

function showTeamHistory($curTeam, $tally_name) {
    if (@$_GET['range'] == 'all') {
        $range = 'all';
    } else {
        $range = get_integer_param($_GET, 'range', 30, 1, null);
    }

    $choices = array();
    foreach ( array( 7, 14, 30, 60, 365, 'all' ) as $d )
    {
        $text =
            ($d == 'all')
            ? _('Lifetime')
            : sprintf( _('Last %d Days'), $d );
        $choices[] = "<a href='tdetail.php?tid=".$curTeam['id']."&tally_name=$tally_name&range=$d'>$text</a>";
    }
    $choices_str = join( $choices, ' | ' );

    $image_url = $GLOBALS['code_url']."/stats/jpgraph_files/tallyboard_deltas.php?tally_name=$tally_name&holder_type=T&holder_id=".$curTeam['id']."&days_back=$range";

    $t = new ThemedTable(
        1,
        _("Team Statistics History"),
        array( 'width' => 95 )
    );

    $t->row(
        "<center><br><p>"
        . "<font size='2'>$choices_str</font>"
        . "<p><img src='$image_url' width='600' height='300'></center><br>"
    );

    $t->end();
}

function stripAllString($ttext) {
    return str_replace(array('[b]','[B]','[/b]','[/B]','[i]','[I]','[/i]','[/I]','[p]','[P]','[/p]','[/P]','[lb]','[LB]'),
    array('<b>','<b>','</b>','</b>','<i>','<i>','</i>','</i>','<p>','<p>','</p>','</p>','<br>','<br>'),
    htmlentities(strip_tags($ttext)));
}

function unstripAllString($ttext,$sType) {
    $ttext=str_replace(array('<b>','</b>','<i>','</i>','<p>','</p>','<br>'),
    array('[b]','[/b]','[i]','[/i]','[p]','[/p]','[lb]'),
    $ttext);
    if ($sType==1) {
        $htmlchars = array_flip(get_html_translation_table(HTML_ENTITIES));
        $ttext=strtr($ttext, $htmlchars);
    }
    return $ttext;
}

function showEdit($tname,$ttext,$twebpage,$tedit,$tsid)
{
    global $theme, $teamimages, $userP;

    echo "<form enctype='multipart/form-data' id='mkTeam' name='mkTeam' action='";
    if ($tedit == 1) { echo "new_team.php"; } else { echo "tedit.php"; }
    echo "' method='post' target='_top'>";
    echo "<input type='hidden' name='tsid' value='$tsid'>";
    if($teamimages['avatar'])
        echo "<input type='hidden' name='tavatar' value='".$teamimages['avatar']."'>";
    if($teamimages['icon'])
        echo "<input type='hidden' name='ticon' value='".$teamimages['icon']."'>";
    echo "\n<table border='1' cellspacing='0' cellpadding='3' style='border: 1px solid #111; border-collapse: collapse' width='95%'>";
    echo "<tr bgcolor='".$theme['color_headerbar_bg']."'><td><center><font color='".$theme['color_headerbar_font']."'><b>";
    if ($tedit != 1) { echo _("Edit Team Information")."</b></font></center></td></tr>"; } else { echo _("New Proofreading Team")."</b></font></td></tr>"; }
    echo "\n<tr><td><table border='0' cellspacing='0' cellpadding='0' width='100%'>";
    echo "\n<tr><td width='35%' align='right'><font color='".$theme['color_mainbody_font']."'><b>"._("Team Name")."</b>:&nbsp;</font></td>";
    echo "<td width='65%' align='left'><input type='text' value='" . attr_safe($tname) . "' name='teamname' size='50'>&nbsp;<b><a href=\"JavaScript:newHelpWin('teamname');\">?</a></b></td></tr>";
    echo "\n<tr><td width='35%' align='right'><font color='".$theme['color_mainbody_font']."'><b>"._("Team Webpage")."</b>:&nbsp;</font></td>";
    echo "<td width='65%' align='left'><input type='text' value='$twebpage' name='teamwebpage' size='50'>&nbsp;<b><a href=\"JavaScript:newHelpWin('teamwebpage');\">?</a></b></td></tr>";
    echo "\n<tr><td width='35%' align='right'><font color='".$theme['color_mainbody_font']."'><b>"._("Team Avatar")."</b>:&nbsp;</font></td>";
    echo "<td width='65%' align='left'><input type='file' name='teamavatar' size='50'>&nbsp;<b><a href=\"JavaScript:newHelpWin('teamavatar');\">?</a></b></td></tr>";
    echo "\n<tr><td width='35%' align='right'><font color='".$theme['color_mainbody_font']."'><b>"._("Team Icon")."</b>:&nbsp;</font></td>";
    echo "<td width='65%' align='left'><input type='file' name='teamicon' size='50'>&nbsp;<b><a href=\"JavaScript:newHelpWin('teamicon');\">?</a></b></td></tr>";
    echo "</table></td></tr>";
    echo "\n<tr bgcolor='".$theme['color_navbar_bg']."'><td><center><font color='".$theme['color_navbar_font']."'><b>"._("Team Description")."</b>&nbsp;</font>";
    echo "<b><a href=\"JavaScript:newHelpWin('teamdesc');\">?</a></b><br><textarea name='text_data' cols='40' rows='6'>" . html_safe($ttext) . "</textarea></center><br></td></tr>";

    if ($tedit == 1 && $userP['team_1'] != 0 && $userP['team_2'] != 0 && $userP['team_3'] != 0) {
        echo "\n<tr bgcolor='".$theme['color_mainbody_bg']."'><td><center>"._("You must join the team to create it, which team space would you like to use?")."<br>";
        echo "<select name='tteams' title='".attr_safe(_("Team List"))."'>";
        $teamQuery = mysql_query("SELECT teamname, id FROM user_teams WHERE id IN ({$userP['team_1']}, {$userP['team_2']}, {$userP['team_3']})");
        while ($row = mysql_fetch_assoc($teamQuery)) {
            echo "<option value='".$row['id']."'>".html_safe(unstripAllString(strip_tags($row['teamname']),1))."</option>";
        }
        echo "</select></center></td></tr>";
    } else {
        echo "<input type='hidden' name='teamall' value='1'>";
    }

    if($tedit == 1) {
        echo "\n<tr bgcolor='".$theme['color_headerbar_bg']."'><td><center>";
        echo "<input type='submit' name='mkPreview' value='".attr_safe(_("Preview Team Display"))."'>&nbsp;&nbsp;&nbsp;";
        echo "<input type='submit' name='mkMake' value='".attr_safe(_("Make Team"))."'>&nbsp;&nbsp;&nbsp;";
        echo "<input type='submit' name='mkQuit' value='".attr_safe(_("Quit"))."'>";
        echo "</center></td></tr></table></form>";
    } else {
        echo "\n<tr bgcolor='".$theme['color_headerbar_bg']."'><td><center>";
        echo "<input type='submit' name='edPreview' value='".attr_safe(_("Preview Changes"))."'>&nbsp;&nbsp;&nbsp;";
        echo "<input type='submit' name='edMake' value='".attr_safe(_("Save Changes"))."'>&nbsp;&nbsp;&nbsp;";
        echo "<input type='submit' name='edQuit' value='".attr_safe(_("Quit"))."'>";
        echo "</center></td></tr></table></form>";
    }
}


function uploadImages($preview,$tid,$type)
{
    global $team_avatars_dir, $team_icons_dir;
    $teamimages = array(
        'avatar' => NULL,
        'icon' => NULL,
    );

    if (!empty($_FILES['teamavatar']['tmp_name']) && ($type == "both" || $type == "avatar")) {
        if (strtolower(substr($_FILES['teamavatar']['name'], -4)) == ".png" || strtolower(substr($_FILES['teamavatar']['name'], -4)) == ".jpg" || strtolower(substr($_FILES['teamavatar']['name'], -4)) == ".gif") {
            if ($_FILES['teamavatar']['size'] > 2097152) {
                echo "<center><br><b><font color='#ff0000'>"._("The avatar uploaded is too large.  Please limit the file size to 2MB or less.")."</font></b></center>";
                return;
            }
            $avatarID = uniqid("avatar_").substr($_FILES['teamavatar']['name'], -4);
            $upload_avatar_dir = "$team_avatars_dir/$avatarID";
            move_uploaded_file($_FILES['teamavatar']['tmp_name'], $upload_avatar_dir);
            if ($preview != 1) {
                mysql_query("UPDATE user_teams SET avatar='$avatarID' WHERE id = $tid") or die(mysql_error());
            }
            $teamimages['avatar'] = $avatarID;
        } else {
            echo "<center><br><b><font color='#ff0000'>"._("The avatar uploaded must be either a JPEG, GIF, or PNG file.")."</font></b></center>";
            return;
        }
    }

    if (!empty($_FILES['teamicon']['tmp_name']) && ($type =="both" || $type == "icon")) {
        if (strtolower(substr($_FILES['teamicon']['name'], -4)) == ".png" || strtolower(substr($_FILES['teamicon']['name'], -4)) == ".jpg" || strtolower(substr($_FILES['teamicon']['name'], -4)) == ".gif") {
            if ($_FILES['teamicon']['size'] > 1048576) {
                echo "<center><br><b><font color='#ff0000'>"._("The icon uploaded is too large. Please limit the file size to 1MB or less.")."</font></b></center>";
                return;
            }
            $iconID = uniqid("icon_").substr($_FILES['teamicon']['name'], -4);
            $upload_icon_dir = "$team_icons_dir/$iconID";
            move_uploaded_file($_FILES['teamicon']['tmp_name'], $upload_icon_dir);
            if ($preview != 1) {
                mysql_query("UPDATE user_teams SET icon='$iconID' WHERE id = $tid");
            }
            $teamimages['icon'] = $iconID;
        } else {
            echo "<center><br><b><font color='#ff0000'>"._("The icon uploaded must be either a JPEG, GIF, or PNG file.")."</font></b></center>";
            return;
        }
    }

    deleteImages();
    return $teamimages;
}

function deleteImages()
{
    global $team_avatars_dir, $team_icons_dir;
    $oneHourAgo = time() - 600;

    $files_to_always_keep = array(
        "avatar_default.png",
        "avatar_default2.png",
        "dp_avatar.png",
        "icon_default.png",
        "icon_default2.png",
        "dp_icon.png",
    );

    // Delete unused avatars
    $result = mysql_query("SELECT id,avatar FROM user_teams");
    while ($row = mysql_fetch_assoc($result)) {
        $id = $row['id'];
        $activeAvatars[$id] = $row['avatar'];
    }

    $avatars = get_filelist($team_avatars_dir);
    $unused_avatars = array_diff($avatars, $activeAvatars, $files_to_always_keep);
    foreach($unused_avatars as $file)
    {
        if (filemtime("$team_avatars_dir/$file") <= $oneHourAgo)
        {
            unlink("$team_avatars_dir/$file");
        }
    }

    //Delete unused icons
    $result = mysql_query("SELECT id,icon FROM user_teams");
    while ($row = mysql_fetch_assoc($result)) {
        $id = $row['id'];
        $activeIcons[$id] = $row['icon'];
    }

    $icons = get_filelist($team_icons_dir);
    $unused_icons = array_diff($icons, $activeIcons, $files_to_always_keep);
    foreach($unused_icons as $file)
    {
        if (filemtime("$team_icons_dir/$file") <= $oneHourAgo)
        {
            unlink("$team_icons_dir/$file");
        }
    }
}
// vim: sw=4 ts=4 expandtab