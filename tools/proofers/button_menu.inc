<?php
include_once($relPath.'site_vars.php');
include_once($relPath.'prefs_options.inc');
include_once($relPath.'button_defs.inc');
include_once($relPath.'misc.inc');
include_once('PPage.inc');

function echo_button_menu( $ppage )
{
    global $userP;
    global $proofreading_font_faces, $proofreading_font_sizes;
?>
<script language="javascript">
function refuseNonNumeric(event) {
    // Deal with the IE/Firefox/WebKit keycode mess...
    var e = event || window.event;
    var key = event.which || event.keyCode || window.event.keyCode;

    // Allow accelerator keys through (for navigation/control) even if we have focus
    if (e.ctrlKey || e.altKey || e.metaKey) return true;

    // Allow backspace, tab, delete, left and right cursor (for editing and navigation)
    if (key == 8 || key == 9 || key == 46 || key == 37 || key == 39) return true;

    // Finally, allow only the 0-9 keys and only if shift isn't pressed
    return (key >= 48 && key <= 57 && !e.shiftKey);
}
</script>
<A
HREF="#" accesskey="," onfocus="scrollImage('left')" onclick="scrollImage('left')"></A><A
HREF="#" accesskey="." onfocus="scrollImage('right')" onclick="scrollImage('right')"></A><A
HREF="#" accesskey="'" onfocus="scrollImage('up')" onclick="scrollImage('up')"></A><A
HREF="#" accesskey="/" onfocus="scrollImage('down')" onclick="scrollImage('down')"></A><select
name="fntFace" ID="fntFace"  class="dropsmall" title="<?php 
    echo attr_safe(_("Change Font Face"));
?>" onChange="top.changeFontFamily(this.options[this.selectedIndex].value, this.options[this.selectedIndex].text)"><?php
    $current_font_i = $userP['i_layout']==1 ? $userP['v_fntf'] : $userP['h_fntf'];
    foreach($proofreading_font_faces as $index => $font)
    {
        if($font == '')
            $font = BROWSER_DEFAULT_STR;

        $optP = "<option \r\n value=\"$index\"";
        if($index == $current_font_i)
            $optP .= " selected";
        $optP .= ">" . $font . "</option>";
        echo $optP;
    }
?></select><select
name="fntSize" ID="fntSize"  class="dropsmall" title="<?php 
    echo attr_safe(_("Change Font Size"));
?>" onChange="top.changeFontSize(this.options[this.selectedIndex].value, this.options[this.selectedIndex].text)"><?php
    $current_size_i = $userP['i_layout']==1 ? $userP['v_fnts'] : $userP['h_fnts'];
    foreach($proofreading_font_sizes as $index => $size)
    {
        if($size == '')
            $size = BROWSER_DEFAULT_STR;

        $optP = "<option \r\n value=\"$index\"";
        if($index == $current_size_i)
            $optP .= " selected";
        $optP .= ">$size</option>";
        echo $optP;
    }
?></select>
<input
type="text" value="<?php
  if ($userP['i_layout']==1)
    {echo $userP['v_zoom'];}
  else{echo $userP['h_zoom'];}
?>" name="zmSize" ID="zmSize" class="boxnormal bottom-align" size="3" title="<?php 
    # xgettext:no-php-format
    echo attr_safe(_("Input Zoom %"));
?>" onkeydown="return refuseNonNumeric(event)"><?php

echo_button(SET_IMAGE_ZOOM,'a');
echo "\n";

// Doesn't work in IE:
// echo_button(SET_IMAGE_ACTUAL,'a');

?>
<a href="#"
onclick="top.proofframe.imageframe.document.scanimage.src='<?php echo $ppage->url_for_image(TRUE); ?>'"><?php echo_img(REFRESH_IMAGE,'a'); ?></a>
<?php

echo_button(REVERT_TO_ORIGINAL,'a');
echo "\n";

  if ( $ppage->can_be_reverted_to_last_save() )
  {
    echo_button(REVERT_TO_LAST_SAVE,'a');
  }
  else
  {
    echo_img(REVERT_TO_LAST_SAVE_DISABLED,'a');
  }

?>
<a href="#" onclick="top.showNW()"><?php echo_img(SHOW_ALL_TEXT,'a'); ?></a>
<a href="#" onclick="previewControl.show()"><?php echo_img(PREVIEW,'a'); ?></a>
<a
href="<?php echo $ppage->url_for_project_comments(TRUE); ?>" TARGET="viewcomments"><?php echo_img(PROJECT_COMMENTS,'a'); ?></a>
<?php

echo_button(SPELL_CHECK,'a');
echo "\n";
echo_button(CHANGE_LAYOUT,'a');
echo "\n";
echo_button(RETURN_PAGE,'a');
echo "\n";

echo_button(REPORT_BAD_PAGE,'a');
echo "\n";

echo_button(SAVE_AS_IN_PROGRESS,'a');
echo "\n";

echo_button(SAVE_AS_DONE_AND_PROOF_NEXT,'a');
echo "\n";

echo_button(SAVE_AS_DONE_AND_QUIT,'a');
echo "\n";
echo_button(QUIT,'a');
}
?>
