<?php
$relPath='../../../pinc/';
include_once($relPath.'base.inc');

header('Content-type: text/plain');

// ------------------------------------------------------------

echo "Increasing size of usersettings.settings column..\n";
$sql = "
    ALTER TABLE usersettings
        CHANGE COLUMN setting setting VARCHAR(64) NOT NULL DEFAULT '';
";

echo "$sql\n";

mysqli_query(DPDatabase::get_connection(), $sql) or die( mysqli_error(DPDatabase::get_connection()) );

// ------------------------------------------------------------

echo "\nDone!\n";

// vim: sw=4 ts=4 expandtab
