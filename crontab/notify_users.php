<?php
$relPath="./../pinc/";
include_once($relPath.'base.inc');
include_once($relPath.'maybe_mail.inc');
include_once($relPath.'misc.inc');
include_once($relPath.'forum_interface.inc');

// check that caller is localhost or bail
if(!requester_is_localhost())
    die("You are not authorized to perform this request.");

$old_date = time() - 13176000; // 30 days less than 1/2 a year.

$reset_password_url = get_reset_password_url();

$result = mysqli_query(DPDatabase::get_connection(), "SELECT * FROM `users` WHERE t_last_activity < $old_date AND active ='yes'");
$numrows = mysqli_num_rows($result);

while ($row = mysqli_fetch_assoc($result))
{
    $username = $row["username"];
    $real_name = $row["real_name"];
    $email = $row["email"];
    $email_updates = $row["email_updates"];
    if ($email_updates) {
        maybe_mail("$email", "$site_name: Inactive Account $username",
             "Hello $real_name,\n\n".
"This is an automated message and your only e-mail reminder that your account on the
 $site_name site ($code_url/) has been inactive
for over 5 months now. In order to show a valid number of active members for our
site, we will be marking this account as inactive a month from today if you do not
log into the site.\n\n
If you wish to receive no more mailings from us, you need to do nothing else and
this account will be marked as inactive. If you have forgotten your password,
visit ($reset_password_url) to have
it reset. We hope you care to join us, much has changed since you last saw us.\n\n
$site_signoff");
    }
}

if($numrows)
{
    echo "notify_users.php attempted to email $numrows users with a 5 month inactivity warning";
}
