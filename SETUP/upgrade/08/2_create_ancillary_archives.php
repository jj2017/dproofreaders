<?php

// Create page_events and wordcheck_events tables in the archive db,
// and populate them (well, really just page_events at this point)
// by moving data for archived projects from the main db.

$relPath='../../../pinc/';
include_once($relPath.'base.inc');
include_once($relPath.'archiving.inc');
// connect.inc (via base.inc) include()s udb_user.php but only in a local scope, so we
// need to include it again to place $archive_db_name in this scope.
include($relPath.'udb_user.php'); // $archive_db_name

error_reporting(E_ALL);

mysqli_query(DPDatabase::get_connection(), "
    CREATE TABLE $archive_db_name.page_events
    LIKE page_events
") or die(mysqli_error(DPDatabase::get_connection()));

mysqli_query(DPDatabase::get_connection(), "
    CREATE TABLE $archive_db_name.wordcheck_events
    LIKE wordcheck_events
") or die(mysqli_error(DPDatabase::get_connection()));

// When upgrading, db server doesn't need to recover between projects:
$archival_recovery_multiplier = 0;

archival_skip_logs_init();

$dry_run = FALSE;

$res = mysqli_query(DPDatabase::get_connection(), "
    SELECT projectid, modifieddate, state
    FROM projects
    WHERE archived = '1'
    ORDER BY modifieddate
") or die(mysqli_error(DPDatabase::get_connection()));
$n_projects = mysqli_num_rows($res);

$i = 0;
while ( $project = mysqli_fetch_object($res) )
{
    $i += 1;

    $prefix = str_pad($i, strlen($n_projects), "0", STR_PAD_LEFT) . "/$n_projects: ";

    $f_modifieddate = strftime('%Y-%m-%d %H:%M:%S',$project->modifieddate);
    echo $prefix, "$project->projectid ($f_modifieddate):\n";

    $indent = str_repeat(' ', strlen($prefix) );
    archive_ancillary_data_for_project_etc( $project, $indent, $dry_run );
}

archival_skip_logs_dump();

// vim: sw=4 ts=4 expandtab
?>
