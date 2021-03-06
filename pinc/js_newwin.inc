<?php

// This file implements the "Launch in New Window" preference.
// This should be the only code that uses $userP['i_newwin']
// (leaving aside code to save/restore/display/update it).

include_once($relPath.'resolution.inc');

function prep_for_links_to_project_pages()
{
    global $userP, $i_resolutions, $code_url;

    // Ensure that the guts of this function are only done once,
    // because don't want to define newProofWin more than once.
    static $this_func_has_been_called = FALSE;
    if ($this_func_has_been_called) return;
    $this_func_has_been_called = TRUE;

    if ( $userP['i_newwin'] == 1 )
    {
        $wSize = explode("x",$i_resolutions[$userP['i_res']*1]);
        $window_width  = $wSize[0] - 20;
        $window_height = $wSize[1] - 80;

        $window_name_base = 
            preg_replace(
                array( "'^[a-z]+:/+'i", "'/.*$'", "'[.]'" ),
                array( "",              "",       "_" ),
                $code_url );

        echo "
            <script type='text/javascript'>
            function newProofWin(winURL)
            {
                newFeatures='toolbar={$userP['i_toolbar']},status={$userP['i_statusbar']},location=0,directories=0,menubar=0,scrollbars=1,resizable=1,width=$window_width,height=$window_height,top=0,left=5';
                nwWin=window.open(winURL,'prooferWin_$window_name_base',newFeatures);
            }
            </script>
        ";
    }
}

function get_onclick_attr_for_link_to_project_page($url)
// Get the 'onclick' attribute (if any) for an <a> element
// that links to a project page.
{
    global $userP;
    if ( $userP['i_newwin'] == 1 )
    {
        $attr = "onclick=\"newProofWin('$url'); return false;\"";
    }
    else
    {
        $attr = '';
    }
    return $attr;
}

// vim: sw=4 ts=4 expandtab
?>
