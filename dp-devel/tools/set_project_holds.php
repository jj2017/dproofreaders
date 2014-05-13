<?php

// Allow authorized users to set/remove holds on a project.

$relPath="./../pinc/";
include_once($relPath.'base.inc');
include_once($relPath.'misc.inc'); // surround_and_join
include_once($relPath.'Project.inc'); // validate_projectID() project_get_hold_states()

require_login();

$projectid = validate_projectID('projectid', @$_POST['projectid']);
$return_uri = urldecode($_POST['return_uri']);

$project = new Project($projectid);

if (!$project->can_be_managed_by_current_user)
{
    echo "<p>", _('You are not authorized to manage this project.'), "</p>\n";
    exit;
}

$delta_ = array(
    'remove' => array(),
    'keep' => array(),
    'add'  => array(),
    'keepout' => array(),
);

$old_hold_states = $project->get_hold_states();

foreach ( $Round_for_round_id_ as $round )
{
    foreach (array('project_waiting_state', 'project_available_state') as $s)
    {
        $state = $round->$s;

        $old_hold = in_array($state, $old_hold_states);

        // In $_POST keys, dots get converted to underscores.
        $new_hold = ( @$_POST[str_replace('.', '_', $state)] == 'on' );

        if ($old_hold)
        {
            if ($new_hold)
            {
                $w = 'keep';
            }
            else
            {
                $w = 'remove';
            }
        }
        else
        {
            if ($new_hold)
            {
                $w = 'add';
            }
            else
            {
                $w = 'keepout';
            }
        }
        $delta_[$w][] = $state;
    }
}

$headers = array(
    'remove' => _("Removing holds for the following states:"),
    'keep'   => _("Keeping holds for the following states:"),
    'add'    => _("Adding holds for the following states:"),
);

foreach( $headers as $w => $header)
{
    $states = $delta_[$w];
    if (count($states))
    {
        echo "<p>$header</p>\n";
        echo "<ul>\n";
        foreach ($states as $state)
        {
            echo "<li>", get_medium_label_for_project_state($state), "</li>\n";
        }
        echo "</ul>\n";

        // -----------------------------------

        $sql = NULL;
        if ($w == 'remove')
        {
            $states_str = surround_and_join( $states, "'", "'", ", " );
            $sql = "
                DELETE FROM project_holds
                WHERE projectid='$projectid'
                    AND state in ($states_str)
            ";
        }
        elseif ($w == 'add')
        {
            $values = '';
            foreach ($states as $state)
            {
                if ($values) $values .= ",\n";
                $values .= "('$projectid', '$state')";
            }
            $sql = "
                INSERT INTO project_holds
                VALUES $values
            ";
        }

        if ($sql)
        {
            // echo $sql;
            mysql_query($sql) or die(mysql_error());
        }
    }
}

echo "<a href='$return_uri'>", _("Click here to return"), "</a>";

// vim: sw=4 ts=4 expandtab
