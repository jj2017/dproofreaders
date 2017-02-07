<?php
$relPath="../pinc/";
include_once($relPath.'base.inc');
include_once($relPath.'metarefresh.inc');
include_once($relPath.'project_states.inc');
include_once($relPath.'project_trans.inc');
include_once($relPath.'theme.inc');
include_once($relPath.'Project.inc');
include_once($relPath.'forum_interface.inc');
include_once($relPath.'misc.inc'); // attr_safe(), endswith()

require_login();

// use:
// $code_url/tools/upload_text.php?project=projectid&curr_state=...

$projectid = validate_projectID('project', @$_REQUEST['project']);
$stage   = $_REQUEST['stage'];
$weeks   = @$_REQUEST['weeks'];
$action  = @$_REQUEST['action'];

$project = new Project($projectid);

// this blurb matches one in remote_file_manager.php
$standard_blurb = _("<b>Note:</b> Please make sure the file you upload is Zipped (not Gzip, TAR, etc.). The file should have the .zip extension, NOT .Zip, .ZIP, etc.");
$submit_blurb = _("After you click the '%s' button, the browser will appear to be slow getting to the next page. This is because it is uploading the file.");
$big_upload_blurb = sprintf(_("<b>Note about big uploads:</b> 
    If you are trying to upload a very big zip file (e.g. 10 Mb)
    and the upload does not succeed, upload a small placeholder zip file 
    instead and email %s for assistance."), $db_requests_email_addr);

if ($stage == 'post_1')
{
    $title = _("Upload post-processed file for verification");
    $intro_blurb = _("This page allows you to upload a post-processed file for verification.");
    $submit_button = _("Upload file");
    $is_file_optional = FALSE;
    $indicator = "_second";
    $new_state = PROJ_POST_SECOND_AVAILABLE;
    $extras = array();
    $back_url = "$code_url/tools/pool.php?pool_id=PP";
    $back_blurb = _("Post-Processing Page");
}
else if ($stage == 'return_1')
{
    $title = _("Return project to the post-processing pool");
    $intro_blurb = _("This page allows you to return the project to the post-processing pool. You can optionally upload a partially post-processed file for another post-processor to pick up and use.");
    $submit_button = _("Return project");
    $is_file_optional = TRUE;
    $indicator = "_first_in_prog_".$pguser;
    $new_state = PROJ_POST_FIRST_AVAILABLE;
    $extras = array();
    $back_url = "$code_url/tools/pool.php?pool_id=PP";
    $back_blurb = _("Post-Processing Page");
}
else if ($stage == 'return_2')
{
    $title = _("Return project to the post-processing verification pool");
    $intro_blurb = _("This page allows you to return the project to the post-processing verification pool. You can optionally upload a partially verified file for another verifier to pick up and use.");
    $submit_button = _("Return project");
    $is_file_optional = TRUE;
    $indicator = "_second_in_prog_".$pguser;
    $new_state = PROJ_POST_SECOND_AVAILABLE;
    $extras = array();
    $back_url = "$code_url/tools/pool.php?pool_id=PPV";
    $back_blurb = _("Post-Processing Verification Page");
}
else if ($site_supports_corrections_after_posting && $stage == 'correct' )
{
    $title = _("Upload corrected edition");
    $intro_blurb = _("This page allows you to upload a corrected version of the completed e-text if you've found an error.");
    $submit_button = _("Upload file");
    $is_file_optional = FALSE;
    $indicator = "_corrections";
    $new_state = PROJ_CORRECT_AVAILABLE;
    $extras = array();
    $back_url = "$code_url/list_etexts.php?x=g";
    $back_blurb = _("Gold List");
    $pre_step_instructions = sprintf(
        _("When making corrections, please read over the entire book and compare your corrections to the <a href='%s'>page images</a> available. Frequently Asked Questions will be developed as this feature is used more. Put any questions in the forums."),
        "$code_url/project.php?id=$projectid&detail_level=3"
    );
}
else if ($stage == 'smooth_avail')
{
    $title = _("Upload file for smooth reading");
    $intro_blurb = _("This page allows you to upload a fully post-processed file for smooth reading.");
    $submit_button = _("Upload file");
    $is_file_optional = FALSE;
    $indicator = "_smooth_avail";
    $new_state = PROJ_POST_FIRST_CHECKED_OUT;
    $extras = array();
    $back_url = "$code_url/project.php?id=$projectid&amp;expected_state=$new_state";
    $back_blurb = _("Project Page");
    $deadline = time() + ($weeks * 60 * 60 * 24 * 7);
}
else if ($stage == 'smooth_done')
{
    $title = _("Upload smooth read version");
    $intro_blurb = _("This page allows you to upload a smooth read version of the project.");
    $submit_button = _("Upload file");
    $is_file_optional = FALSE;
    $indicator = "_smooth_done_".$pguser;
    $new_state = PROJ_POST_FIRST_CHECKED_OUT;
    $extras = array();
    $back_url = "$code_url/project.php?id=$projectid&amp;expected_state=$new_state";
    $back_blurb = _("Project Page");
    $deadline = time() + ($weeks * 60 * 60 * 24 * 7);

}
else if (!isset($stage))
{
    // this may be due to a timeout when uploading big files.
    include_once($relPath.'slim_header.inc');
    
    slim_header(_("Upload failed"));
    
    echo "<p>" . _("The upload failed.") . "</p>\n";
    echo "<p>$big_upload_blurb</p>";
    
    echo "<p>" . sprintf(_("Please go <a href='%s'>back</a> and try uploading 
        the original again or uploading a smaller placeholder instead."), 
        "javascript:history.back()") . "</p>";
    
    exit;
}

else
{
    echo "Don't know how to handle stage='$stage'<br>\n";
    return;
}

if (!isset($action))
{
    // Present the upload page.

    output_header($title);

    echo "<h1>$title</h1>";
    echo "<h2>" . sprintf("Project: %s", $project->nameofwork) . "</h2>";
    echo "<p>$intro_blurb</p>";
    if(isset($pre_step_instructions))
        echo "<p>$pre_step_instructions</p>";

    echo "<form action='upload_text.php' method='POST' enctype='multipart/form-data'>\n";
    echo "<input type='hidden' name='project' value='$projectid'>\n";
    echo "<input type='hidden' name='stage' value='$stage'>\n";
    echo "<input type='hidden' name='weeks' value='$weeks'>\n";
    echo "<input type='hidden' name='action' value='1'>\n";
    echo "<input type='hidden' name='MAX_FILE_SIZE' value='25165824'>\n";

    echo "<p>" . _("Follow these steps:") . "</p>\n";
    echo "<ol>\n";

    echo "<li>";
    if($is_file_optional) {
        echo _("(optional) Select a zipped file to upload:");
    } else {
        echo _("Select a zipped file to upload:");
    }
    echo "<br>\n";
    echo "<input type='file' name='files[]' size='25' maxsize='50'>\n";
    echo "<p>$standard_blurb</p>";
    echo "<p>$big_upload_blurb</p>\n";
    echo "</li>\n";

    if ($stage != 'smooth_done') {
        echo "<li>";
        if ($stage != 'smooth_avail') {
            echo _("(optional) Leave comments for the next person who checks out this project:");
        } else {
            echo _("Leave instructions for smooth readers:");
        }
        echo "<br>\n";
        echo "<textarea style='margin-bottom: 1em;' name='postcomments' cols='75' rows='10'></textarea>\n";
        echo "</li>\n";
    }

    echo "<li>";
    // TRANSLATORS: %1$s is replaced with a form <input> containing separately translated text like 'Upload file'
    echo sprintf(_('%1$s or return to the <a href="%2$s">%3$s</a>.'),
        sprintf('<input type="submit" value="%s">', attr_safe($submit_button)),
        $back_url, $back_blurb);
    echo "<p>" . sprintf($submit_blurb,$submit_button) . "</p>";
    echo "</li>\n";
    echo "</ol>";

    echo "</form>\n";
}
else
{
    // Handle a submission from the upload page.

    // if files have been uploaded, process them
    // mangle the postcomments

    // make reasonably sure script does not timeout on large file uploads
    set_time_limit(14400);
    $path_to_file = "$projects_dir/$projectid";

    $files = $_FILES['files'];

    // do some checks. File must exist (except if we are returning to PP 
    // or PPV available.
    // if we have a file, we need its name to end in .zip, and we need
    // it to have non zero size.
    // and there must be only one file.

    $returning_to_pool = ('return_1' == $stage || 'return_2' == $stage);
    $need_file = !$returning_to_pool;    // in future, may be other conditions for this
    $file_count = count($files['name']);
    if ($file_count > 1) {
        echo _("You may only upload one file");
        exit();
    }
    // file_count never seems to be 0, but we keep the check in, just in case
    $have_file = (1 == $file_count && strlen($files['name'][0]) > 0);

    if ($need_file && ! $have_file) {
        echo _("You must upload a file");
        exit();
    }

    if ($have_file) {       // we have a file now. do some more checks.
        if (substr($files['name'][0], -4) != ".zip") {
            echo _("Invalid Filename");
            exit();
        }
        if (0 == $files['size'][0]) {
            echo _("File is empty");
            exit();
        }  
    }
    
    // this is ridiculous, why not just include the "/" to start with?
    if (!endswith($path_to_file, "/"))
    {
        $path_to_file = $path_to_file."/";
    }

    function ensure_path_is_unused( $path )
        // Ensure that nothing exists at $path.
        // (If something's there, rename it.)
        // EXCEPT: let people overwrite their finished SR files as often as they want
        {
            global $stage, $db_requests_email_addr;
            
            if ( file_exists($path) )
            {

                if (($stage != 'smooth_done') AND ($stage != 'smooth_avail')){

                    $bak = "$path.bak";
                    ensure_path_is_unused( $bak );
                    $success = rename( $path, $bak );
                    if (!$success)
                    {
                        // It will already have printed a warning.
                        echo sprintf(
                            _("A problem occurred with your upload. Please email %s for assistance, and include the text of this page."),
                            $db_requests_email_addr );
                        exit;
                    }
                } else {

                    unlink($path);
                }
            }
        }

    if ($have_file) {
        // replace filename
        $zipext = ".zip";
        $name = $projectid.$indicator.$zipext;
        $location = $path_to_file.$name;
        ensure_path_is_unused( $location );
        copy($files['tmp_name'][0],$location);
        unlink($files['tmp_name'][0]);
    }
    // we've put the file in the right place.
    // now let's deal with the postcomments.
    // we construct the bit that's going to be added on to the existing postcomments.
    // if we're returning to available, and the user hasn't loaded a file, and not
    // entered any comments, we don't bother.
    // Otherwise, we add a divider, time stamp, user name, and the name of the file
    $postcomments = @$_POST['postcomments'];
    $divider = "\n----------\n".date("Y-m-d H:i");
    if ($have_file) {
        $divider .= "  ".$name." "._("uploaded by")." ";
    }
    else if ($returning_to_pool) {
        $divider .= " "._("returned by")." ";
    }
    else {
        $divider .= " "; // this shouldn't actually happen
    }
    $divider .= $pguser."\n";
    if (strlen($postcomments)>0 || $have_file) {
        $postcomments = $divider . $postcomments;
    }

    $error_msg = project_transition( $projectid, $new_state, $pguser, $extras );
    if ($error_msg)
    {
        echo "$error_msg<br>\n";
    }

    // special handling for smooth reading, which does not involve a state change
    // but still needs some changes recorded in project table
    // the comments get recorded even if it's just a replacement
    if ($stage == 'smooth_avail') {
        $smoothread_deadline = '';
        if ($weeks != "replace") {
            $smoothread_deadline = "smoothread_deadline = $deadline, ";
        }
        $qstring = sprintf("
            UPDATE projects
            SET $smoothread_deadline
                postcomments = CONCAT(postcomments, '%s')
            WHERE projectid = '$projectid'
        ", mysql_real_escape_string($postcomments));
        $qry =  mysql_query($qstring);

        if ( $weeks == "replace" )
        {
            log_project_event( $projectid, $pguser, 'smooth-reading', 'text replaced' );
        }
        else
        {
            log_project_event( $projectid, $pguser, 'smooth-reading', 'text available', $deadline );
        }

        notify_project_event_subscribers( $project, 'sr_available' );

        if ( $auto_post_to_project_topic )
        {
            // Add an auto-post to the project's discussion topic.
            $project->ensure_topic();
            topic_add_post(
                $project->topic_id,
                "Project made available for smooth-reading",
                "The project has just been made available for smooth-reading for $weeks weeks."
                    . "\n\n"
                    . "(This post is automatically generated.)",
                '[Smooth Reading Monitor]',
                FALSE
            );
        }
    }

    // let them know file uploaded and send back to the right place
    $msg1 = _("File uploaded. Thank you!");
    $msg2 = _("Project returned to pool");
    if ($have_file && $returning_to_pool) {
        $msg = $msg1."\n".$msg2;
    }
    else if ($have_file) {
        $msg = $msg1;
    }
    else if  ($returning_to_pool) {
        $msg = $msg2;
    }
    else {
        $msg = _("This shouldn't happen. No file upload and not returning to pool.");
    }
    metarefresh(1, $back_url, $msg, $msg);
}

// vim: sw=4 ts=4 expandtab