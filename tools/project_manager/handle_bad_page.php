<?php
$relPath="./../../pinc/";
include_once($relPath.'base.inc');
include_once($relPath.'project_states.inc');
include_once($relPath.'theme.inc');
include_once($relPath.'DPage.inc');
include_once($relPath.'Project.inc');
include_once($relPath.'stages.inc');
include_once($relPath.'forum_interface.inc');
include_once($relPath.'project_edit.inc');
include_once($relPath.'misc.inc'); // attr_safe(), html_safe()
include_once('page_table.inc');  // page_state_is_a_bad_state()

require_login();

$projectid = validate_projectID('projectid', @$_REQUEST['projectid']);
$image     = validate_page_image_filename('image', @$_REQUEST['image']);
$modify    = array_get($_REQUEST, 'modify', '');
$cancel    = array_get($_POST, 'cancel', '');
$prev_text = array_get($_POST, 'prev_text', NULL);
$prevtext_column = array_get($_POST, 'prevtext_column', NULL);
$resolution = array_get($_POST, 'resolution', NULL);

if(user_can_edit_project($projectid) != USER_CAN_EDIT_PROJECT)
{
    die("You are not authorized to manage this project.");
}

// If the user hit a cancel button, return them to the starting form
if($cancel)
    $modify = '';

if (!$resolution) {
    //Find out information about the bad page report
    $result = mysql_query("SELECT * FROM $projectid WHERE image='$image'");
    $page = mysql_fetch_assoc($result);
    $state  = $page['state'];
    $b_User = $page['b_user'];
    $b_Code = $page['b_code'];

    $project = new Project($projectid);
    
    $round = get_Round_for_page_state($state);

    // It's a bit messy to have this here,
    // since it reiterates stuff that appears in other files,
    // but this page is kind of messy to begin with.
    // It'll get cleaned up eventually.
    for ( $prev_round_num = $round->round_number-1; $prev_round_num > 0; $prev_round_num-- )
    {
        $r = get_Round_for_round_number($prev_round_num);
        if ( $page[$r->user_column_name] != '' )
        {
            $prevtext_column = $r->text_column_name;
            break;
        }
    }
    if ( $prev_round_num == 0 )
    {
        $prevtext_column = 'master_text';
    }

    // Is it a bad page report, or are we merely fixing an ordinary page
    $is_a_bad_page = page_state_is_a_bad_state($state);
    if ($is_a_bad_page)
    {
        $header = _("Bad Page Report");
    }
    else
    {
        $header = _("Fix Page");
    }
    
    //Display form
    output_header($header);

    echo "<h1>$header</h1>";

    echo "<p>";
    echo "<b>" . _("Project") . ":</b> {$project->nameofwork}<br>";
    echo "<b>" . _("Page") . ":</b> $image<br>";
    echo "<b>" . _("State") . ":</b> $state<br>";
    echo "<b>" . _("Originals") . "</b>: ";
    echo "<a href='downloadproofed.php?project=$projectid&image=$image&round_num=$prev_round_num' target='_new'>" . _("View Text") . "</a>";
    echo " | ";
    echo "<a href='displayimage.php?project=$projectid&imagefile=$image' target='_new'>" . _("View Image") . "</a>";
    echo "</p>";

    if($is_a_bad_page)
    {
        echo "<p>" . _("This page has been marked bad by the following user.") . "</p>";

        echo "<p>";
        if (!empty($b_User))
        {
            $contact_url = get_url_to_compose_message_to_user($b_User);
            $contact_url = attr_safe($contact_url);
            echo "<b>" . _("User") . ":</b> $b_User ".
                "(<a href='$contact_url'>" . _("Private Message") . "</a>)<br>";
        }

        if (!empty($b_Code))
        {
            echo "<b>" . _("Reason") . ":</b> {$PAGE_BADNESS_REASONS[$b_Code]}</br>";
        }
        echo "</p>";
    }

    $show_resolution_form = TRUE;
    //Determine if modify is set & if so display the form to either modify the image or text
    if($modify == "text")
    {
        if($prev_text == NULL)
        {
            $prev_text = $page[$prevtext_column];
            show_text_update_form($projectid, $image, $prev_text, $prevtext_column);
            $show_resolution_form = FALSE;
        }
        else
        {
            Page_modifyText( $projectid, $image, $prev_text, $prevtext_column, $pguser );
            echo "<p><b>"._("Update of Text from Previous Round Complete!")."</b></p>";
        }
    }
    elseif($modify == "image")
    {
        if(!count($_FILES))
        {
            show_image_update_form($projectid, $image);
            $show_resolution_form = FALSE;
        }
        else
        {
            update_image($projectid, $image);
        }
    }

    if($show_resolution_form)
        show_resolution_form($projectid, $image, $state, $prev_round_num, $is_a_bad_page);

} else {
    //Get variables passed from form
    $state = get_enumerated_param($_POST, 'state', null, $PAGE_STATES_IN_ORDER);

    //If the PM fixed the problem or stated the report was invalid update the database to reflect
    if (($resolution == "fixed") || ($resolution == "invalid")) {
        $round = get_Round_for_page_state($state);
        Page_eraseBadMark( $projectid, $image, $round, $pguser );
    }

    //Redirect the user back to the project detail page.
    header("Location: $code_url/project.php?id=$projectid&detail_level=4");
}

#----------------------------------------------------------------------------

function show_resolution_form($projectid, $image, $state, $prev_round_num, $is_a_bad_page)
{
    global $theme;

    if($is_a_bad_page)
        echo "<h2>" . _("Resolve bad page") . "</h2>";

    echo "<form action='handle_bad_page.php' method='post'>";
    echo "<input type='hidden' name='projectid' value='$projectid'>";
    echo "<input type='hidden' name='image' value='$image'>";
    echo "<input type='hidden' name='state' value='$state'>";
    echo "<br><div align='center'><table bgcolor='".$theme['color_mainbody_bg']."' border='1' cellspacing='0' cellpadding='0' style='border: 1px solid #111; border-collapse: collapse'>";

    echo "<tr><td bgcolor='$theme[color_logobar_bg]' align='left'>";
    echo "<strong>"._("Modify").":</strong></td>";
    echo "<td bgcolor='#ffffff' align='center'>";
    echo "<a href='handle_bad_page.php?projectid=$projectid&image=$image&modify=text'>"._("Text from Previous Round")."</a>";
    echo " | ";
    echo "<a href='handle_bad_page.php?projectid=$projectid&image=$image&modify=image'>"._("Original Image")."</a>";
    echo "</td></tr>\n";
    echo "<tr><td bgcolor='$theme[color_logobar_bg]' align='left'>";

    if ($is_a_bad_page) {
        echo "<strong>"._("What to do").":&nbsp;&nbsp;</strong></td>";
        echo "<td bgcolor='#ffffff' align='center'>";
        echo "<input name='resolution' value='fixed' type='radio'>"._("Fixed")."&nbsp;";
        echo "<input name='resolution' value='invalid' type='radio'>"._("Invalid Report")."&nbsp;";
        echo "<input name='resolution' value='unfixed' checked type='radio'>"._("Not Fixed")."&nbsp;";
        echo "</td></tr>\n";
    }
    else
    {
        echo "<input name='resolution' value='something' type='hidden'>";
        // Doesn't really matter what the value is.
    }

    echo "<tr><td bgcolor='$theme[color_headerbar_bg]' colspan='2' align='center'>";
    echo "<input type='submit' value='" . attr_safe(_("Continue")) . "'>";
    echo "</td></tr></table></div></form><br><br>";
}

function show_text_update_form($projectid, $image, $prev_text, $prevtext_column)
{
    echo "<h2>" . _("Update page text") . "</h2>";

    echo "<form action='handle_bad_page.php' method='post'>";
    echo "<input type='hidden' name='modify' value='text'>";
    echo "<input type='hidden' name='projectid' value='$projectid'>";
    echo "<input type='hidden' name='image' value='$image'>";
    echo "<input type='hidden' name='prevtext_column' value='$prevtext_column'>";
    // TRANSLATORS: %s is the image name.
    echo sprintf(_("The textarea below contains the text from the previous round for %s."), $image) . "<br>";
    echo _("You may use it as-is, or insert other replacement text for this page:") . "<br>";
    // newline after <textarea> needed to prevent the text box from eating the first blank line
    echo "<textarea name='prev_text' cols=70 rows=10>\n";
    // SENDING PAGE-TEXT TO USER
    echo html_safe($prev_text);
    echo "</textarea><br><br>";
    echo "<input type='submit' value='"
        . attr_safe(_("Update Text From Previous Round")) . "'> ";
    echo "<button type='submit' name='cancel' value='cancel'>" . _("Cancel") . "</button>";
    echo "</form>";
}

function show_image_update_form($projectid, $image)
{
    echo "<h2>" . _("Update page image") . "</h2>";

    echo "<form enctype='multipart/form-data' action='handle_bad_page.php' method='post'>";
    echo "<input type='hidden' name='modify' value='image'>";
    echo "<input type='hidden' name='projectid' value='$projectid'>";
    echo "<input type='hidden' name='image' value='$image'>";
    // TRANSLATORS: %s is the image name.
    echo sprintf(_("Select an image to upload and replace %s with:"), $image) . "<br>";
    echo "<input type='file' name='image_upload' size=30><br><br>";
    echo "<input type='submit' value='" . attr_safe(_("Update Original Image")) . "'> ";
    echo "<button type='submit' name='cancel' value='cancel'>" . _("Cancel") . "</button>";
    echo "</form>";
}

function update_image($projectid, $image)
{
    global $projects_dir;

    $org_image_ext = substr($image, -4);
    $org_image_basename = basename($image, $org_image_ext);
    $tmp_image_ext = substr($_FILES['image_upload']['name'], -4);

    if ( $tmp_image_ext == ".png" || $tmp_image_ext == ".jpg" ) {
        if ( $tmp_image_ext == $org_image_ext ) {
            copy($_FILES['image_upload']['tmp_name'],"$projects_dir/$projectid/$image") or die("Could not upload new image!");
            echo "<p><b>" . sprintf(_("Update of Original Image %s Complete!"), $image) . "</b></p>";
        } else {
            echo "<p class='error'>"._("Image NOT updated.<br>");
            echo sprintf(_("The uploaded file type (%1\$s) does not match the original file type (%2\$s)."),
                $tmp_image_ext, $org_image_ext) . "</p>";
            echo "<p>" . sprintf(_("Click <a href='%s'>here</a> to return."),
                "handle_bad_page.php?projectid=$projectid&image=$image&modify=image") . "</p>";
            exit;
        }
    } else {
        echo "<p class='error'>"._("The uploaded file must be a PNG or JPG file!") . "</p>";
        echo "<p>". sprintf(_("Click <a href='%s'>here</a> to return."),
                "handle_bad_page.php?projectid=$projectid&image=$image&modify=image") . "</p>";
        exit;
    }
}

// vim: sw=4 ts=4 expandtab
