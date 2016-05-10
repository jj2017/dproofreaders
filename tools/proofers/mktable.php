<?php
// not including base.inc as this has no external dependencies
$relPath="../../pinc";
include_once($relPath."/misc.inc");

$charset="UTF-8";

define("ARRAY_PAD_FRONT", -1);
define("ARRAY_PAD_BACK",   1);
define("ARRAY_PAD_BOTH",   0);

mb_internal_encoding($charset);

$row = array_get($_POST, 'row', 1);
$col = array_get($_POST, 'col', 1);
$bord = array_get($_POST, 'border', 1);
$trim = (array_get($_POST, 'trim', 'off') == 'on');
$clear = array_get($_POST, 'clear', 0);
?>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<?php echo $charset; ?>">
<title>Table Maker</title>
</head>
<body>
<form method="POST">
<input type="submit" value="Create">
<input type="text" name="row" value="<?php echo $row; ?>" size="2"> rows and
<input type="text" name="col" value="<?php echo $col; ?>" size="2"> columns table
<select name="border">
<option value="1"<?php echo $bord?" selected":""; ?>>with</option>
<option value="0"<?php echo $bord?"":" selected"; ?>>without</option>
</select> border;
<input type="checkbox" name="trim"<?php echo $trim?" checked":""; ?>> trim spaces.<br/>
<table>
<tr>
<td>&nbsp;</td>
<?php

if($clear) {
    for($i=0;$i<$row;$i++) {
        $_POST["val$i"]="t";
        for($j=0;$j<$col;$j++) {
            $_POST["al$j"]="l";
            $_POST["a{$i}_{$j}"]="";
        }
    }
}

$al=array();
for($j=0;$j<$col;$j++) {
    switch($t=$_POST["al$j"]) {
        case "r":
            $al[$j]=STR_PAD_LEFT;
            break;
        case "c":
            $al[$j]=STR_PAD_BOTH;
            break;
        default:
            $al[$j]=STR_PAD_RIGHT;
            break;
    }
    $right_align_checked = ($al[$j]==STR_PAD_RIGHT) ? " checked": "";
    $both_align_checked  = ($al[$j]==STR_PAD_BOTH ) ? " checked": "";
    $left_align_checked  = ($al[$j]==STR_PAD_LEFT ) ? " checked": "";
    echo <<<HORIZ_ALIGN
<td align="center">
    <input type="radio" name="al$j" value="l" $right_align_checked>
        <img src="./../../graphics/left.gif" alt="left">
    <input type="radio" name="al$j" value="c" $both_align_checked>
        <img src="./../../graphics/center.gif" alt="center">
    <input type="radio" name="al$j" value="r" $left_align_checked>
        <img src="./../../graphics/right.gif" alt="right">
</td>
HORIZ_ALIGN;
}

echo "</tr>";

$a=array(); $lng=array(); $tll=array(); $val=array();
//$row=5; $col=5;
for($i=0;$i<$row;$i++) {
    switch($t=$_POST["val$i"]) {
        case "b":
            $val[$i]=ARRAY_PAD_FRONT;
            break;
        case "m":
            $val[$i]=ARRAY_PAD_BOTH;
            break;
        default:
            $val[$i]=ARRAY_PAD_BACK;
            break;
    }
    $back_align_checked  = ($val[$i]==ARRAY_PAD_BACK ) ? " checked": "";
    $both_align_checked  = ($val[$i]==ARRAY_PAD_BOTH ) ? " checked": "";
    $front_align_checked = ($val[$i]==ARRAY_PAD_FRONT) ? " checked": "";
    echo <<<VERT_ALIGN
<td valign="middle">
    <input type="radio" name="val$i" value="t" $back_align_checked>
        <img src="./../../graphics/top.gif" alt="top"><br />
    <input type="radio" name="val$i" value="m" $both_align_checked>
        <img src="./../../graphics/middle.gif" alt="middle"><br />
    <input type="radio" name="val$i" value="b" $front_align_checked>
        <img src="./../../graphics/bottom.gif" alt="bottom"><br />
</td>
VERT_ALIGN;

    $a[$i]=array();
    for($j=0;$j<$col;$j++) {
        $name="a{$i}_{$j}";
        $a[$i][$j]=explode("\n",str_replace("\r\n","\n",$_POST[$name]=stripslashes($_POST[$name])));
        foreach($a[$i][$j] as $k=>$v) {
            if($trim)
                $a[$i][$j][$k]=$v=trim($v);
            $t=mb_strlen($v);
            if(!isset($lng[$j]) || $t>$lng[$j])
                $lng[$j]=$t;
        }
        if($trim)
            $a[$i][$j]=array_filter($a[$i][$j],"str_not_empty");
        $t=count($a[$i][$j]);
        if(!isset($tll[$i]) || $t>$tll[$i])
            $tll[$i]=$t;
?>
<td><textarea name="<?php echo $name; ?>" wrap="off">
<?php echo htmlspecialchars($_POST[$name], ENT_NOQUOTES, $charset); ?>
</textarea></td>
<?php
    }
    echo "</tr>";
}
?>
</table>
<input type="submit" value="Draw">
<input type="submit" name="clear" value="Clear">
</form>
<form>
<textarea rows="20" cols="80" wrap="off">
<?php generate_ascii_table($row, $col, $a, $al, $tll, $val, $lng, $bord); ?>
</textarea>
</form>
</body>
</html>
<?php

#----------------------------------------------------------------------------

function generate_ascii_table($row, $col, $a, $al, $tll, $val, $lng, $bord)
{
    global $charset;

    if($bord) hline($col, $lng, $bord);

    for($i=0;$i<$row;$i++) {
        for($j=0;$j<$col;$j++) {
            $a[$i][$j]=array_pad_internal($a[$i][$j],$tll[$i],$val[$i]);
        }
        for($k=0;$k<$tll[$i];$k++) {
            if($bord) echo "|";
            for($j=0;$j<$col-1;$j++) {
                echo htmlspecialchars(mb_str_pad($a[$i][$j][$k],$lng[$j]," ",$al[$j]), ENT_QUOTES, $charset)."|";
            }
            echo htmlspecialchars(mb_str_pad($a[$i][$col-1][$k],$lng[$col-1]," ",$al[$col-1]), ENT_QUOTES, $charset);
            if($bord) echo "|";
            echo "\n";
        }

        if($bord||$i<($row-1))
            hline($col, $lng, $bord);
    }
}

function hline($col, $lng, $bord)
{
    if($bord) echo "+";
    for($j=0;$j<$col-1;$j++)
        echo mb_str_pad("",$lng[$j],"-")."+";
    echo mb_str_pad("",$lng[$col-1],"-");
    if($bord) echo "+";
    echo "\n";
}

function str_not_empty($str)
{
    return $str!=="";
}

function mb_str_pad($input,$pad_length,$pad_string=" ",$pad_type=STR_PAD_RIGHT,$encoding=NULL)
{
    if($encoding==NULL)
        $encoding=mb_internal_encoding();
    $l=mb_strlen($input,$encoding);
    if($pad_length<=$l)
        return $input;

    switch($pad_type) {
        case STR_PAD_LEFT:
            for($i=0;$i<$pad_length-$l;$i++)
                $input=$pad_string.$input;
            break;
        case STR_PAD_RIGHT:
            for($i=0;$i<$pad_length-$l;$i++)
                $input.=$pad_string;
            break;
        case STR_PAD_BOTH:
            for($i=0;$i<floor(($pad_length-$l)/2);$i++)
                $input=$pad_string.$input;
            for($i=0;$i<ceil(($pad_length-$l)/2);$i++)
                $input.=$pad_string;
            break;
        default:
            break;
    }
    return $input;
}

function array_pad_internal($input,$pad_size,$pad_type)
{
    if($pad_type==ARRAY_PAD_BOTH)
        return array_pad(
            array_pad(
                $input,
                ($pad_size-count($input))/2-$pad_size,
                ""
            ),
            $pad_size,
            ""
        );
    else 
        return array_pad($input,$pad_type*$pad_size,"");
}
