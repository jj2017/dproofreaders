<?php
$relPath='./../pinc/';
include_once($relPath.'connect.inc');
include_once($relPath.'project_states.inc');
include_once($relPath.'theme.inc');

$which = get_enumerated_param($_GET, 'which', null, $project_status_descriptors);

new dbConnect();

$psd = get_project_status_descriptor($which);

theme($psd->graphs_title,'header');

echo "<center><h1><i>$psd->graphs_title</i></h1></center>";

echo "<br><br>";

echo "<center><img src=\"jpgraph_files/curr_month_proj.php?which=$which\"></center><br>";
echo "<center><img src=\"jpgraph_files/cumulative_month_proj.php?which=$which\"></center><br>";
echo "<center><img src=\"jpgraph_files/total_proj_graph.php?which=$which\"></center><br>";
echo "<center><img src=\"jpgraph_files/cumulative_total_proj_graph.php?which=$which\"></center><br>";

theme('','footer');

// vim: sw=4 ts=4 expandtab
?>
