<?php

$relPath='../../../pinc/';
include($relPath.'connect.inc');
new dbConnect();

header('Content-type: text/plain');

// ------------------------------------------------------------

echo "Removing unnecessary index on projectid";
$sql = "
    ALTER TABLE smoothread
        DROP INDEX project;
";

echo "$sql\n";

mysql_query($sql) or die( mysql_error() );

// ------------------------------------------------------------

echo "\nDone!\n";


// vim: sw=4 ts=4 expandtab
?>