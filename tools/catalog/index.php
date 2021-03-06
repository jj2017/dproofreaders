<?php
$relPath = '../../pinc/';
include_once($relPath.'base.inc');
include_once($relPath.'project_states.inc');
include_once($relPath.'slim_header.inc');
include_once($relPath.'misc.inc'); // html_safe()

require_login();

$title = sprintf(_("%s Catalog"), $site_name);

slim_header($title);

echo "<h1>$title</h1>";

echo "<p>" . _("This page lists all projects, grouped by state. (We could have other ways of grouping/ordering. We could also have a search interface.) Click on a project title to go to its home page.") . "</p>";

foreach ( $PROJECT_STATES_IN_ORDER as $proj_state )
{
    echo "<h3>";
    echo project_states_text($proj_state);
    echo ":</h3>\n";

    $rows = mysqli_query(DPDatabase::get_connection(),  "SELECT projectid, nameofwork, authorsname, language FROM projects WHERE state='$proj_state'") or die(mysqli_error(DPDatabase::get_connection()));

    if ( mysqli_num_rows($rows) == 0 )
    {
        echo "none<br>\n";
    }
    else
    {
        while( $row = mysqli_fetch_array( $rows ) )
        {
            $projectid   = $row['projectid'];
            $nameofwork  = $row['nameofwork'];
            $authorsname = $row['authorsname'];
            $language    = $row['language'];
            // TRANSLATORS: format is: name_of_work by author (in language)
            echo sprintf(_("<a href='%1\$s'>%2\$s</a> by %3\$s (in %4\$s)"),
                "$code_url/project.php?id=$projectid",
                html_safe($nameofwork),
                html_safe($authorsname),
                html_safe($language)) . "<br>\n";
        }
    }
}

// vim: sw=4 ts=4 expandtab
