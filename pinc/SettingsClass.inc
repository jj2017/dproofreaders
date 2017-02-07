<?php

// $Id$

/**
 * Settings Class.

  Note: Everything marked "Implementation:" is information
        that should remain encapsulated inside the class.

 This class provides access to user settings.
     ******
     Implementation: Initially limited to those stored in the usersettings table.
                     Subsequently expanded to cover "manager", "postprocessor",
                     and "sitemanager" from the "users" table. 
     ******

 There are three setting types:

1. Boolean settings. Return value is True or False.
 Example setting: post_proofer
     ******
    Implementation:
        Setting to True.
            Will ensure that a record exists with "yes" in the value column.
        Setting to anything other than True
            Will ensure that the record is deleted.
        Getting.
            Will return True if there is a record and the value column holds "yes".
            Will return False otherwise.
     ******

2. Value settings. Non-empty string settings.
 Example: R1order (sort order of project listing) Example value: "GenreA" (Genre Ascending)
     ******
    Implementation:
        Setting:
            Will ensure that record is deleted if the provided value is anything
                resolving to Null in PHP (including zero-valued numbers).
            Will ensure that a record exists with the value column set to the string
                value provided (or its PHP auto-conversion-string if other data type.)
        Getting:
            Will provide exactly what's in the value column. (Note this may result
            in something-in/something-else-out if PHP auto-conversion took place.)
     ******

3. Boolean-string-settings (for want of a better name.) These are cases where a record
is inserted to indicate that a boolean value is True for a user for (typically) a project.
Example setting: "taskctr_notice", value "1018"
     ******
    Implementation:
        Setting and Getting: Set and get exactly what's provided.

12/30/04 (DAK) Removed last vestige of a preference for/default of the logged on user.
               The constructor now requires a username argument.
*/
        

class Settings
{
    // Multi-dimensional array containing all user settings. Each setting
    // may have one or more value associated with it. Format is:
    //     $settings_array[$setting] = array($value1, ...);
    private $settings_array = array();

    // username of subject user.
    private $username = NULL;

    public function __construct($name)
    {
        if($name == '')
        {
            // Note that that's a "loose equals", so $name could be:
            // the empty string, the integer zero, the boolean false, or null.
            // Of those, the likeliest is null, indicating that the
            // current http-requestor is a "guest", not a logged-in user.

            // Return a Settings object with an empty $settings_array.
            return;
        }

        $this->username = $name;

        // Query the "usersettings" table and get all the rows for our user.
        // build an array with the settings, to use when somebody asks.
        $sql = sprintf("
            SELECT u.sitemanager,
                   u.manager,
                   u.postprocessor,
                   us.*
            FROM users AS u
            LEFT OUTER JOIN usersettings AS us ON u.username = us.username
            WHERE u.username = '%s'
        ", mysql_real_escape_string($this->username));

        $result = mysql_query($sql);
        // To know whether we've populated fields from 'users' table yet:
        $isUserVals = false;
        while ($row = mysql_fetch_assoc($result))
        {
            // get the onesies from the first row
            // yes, this is more efficient than two separate queries.
            if(!$isUserVals)
            {
                $isUserVals = true;
                if ($row['sitemanager'] == 'yes')
                    $this->_load_value('sitemanager', 'yes');
                if ($row['manager'] == 'yes')
                    $this->_load_value('manager', 'yes');
                if ($row['postprocessor'] == 'yes')
                    $this->_load_value('postprocessor', 'yes');
            }
            // now continue to add values from usersettings table.
            // This could be null values if the outer join matched nothing
            // so check there is something in the "setting" column.
            if($row['setting'])
                $this->_load_value($row['setting'], $row['value']);
        }
        mysql_free_result($result);
    }

    // check if this is an updateable setting in this context.
    private function _isSettable($settingCode)
    {
        switch($settingCode)
        {
            default:
                return true;
            case 'sitemanager':
            case 'manager':
            case 'postprocessor':
                return false;
        }
    }

    // whom do I describe?
    function UserName()
    {
        return $this->username ? $this->username : "[none]";
    }

    // -------------------------------------------------------------------------

    // Setting to True
    public function set_true($settingCode)
    {
        $this->set_boolean($settingCode,TRUE);
    }

    // Setting to False
    public function set_false($settingCode)
    {
        $this->set_boolean($settingCode,FALSE);
    }

    public function set_boolean($settingCode, $boolval)
    // A wrapper around set_value() for boolean values.
    {
        $this->set_value($settingCode, ($boolval ? 'yes' : NULL) );
    }

    // Return True iff the setting exists and its value is 'yes'.
    // Otherwise, return False.
    public function get_boolean($settingCode)
    {
        return ( $this->get_value($settingCode) === 'yes' );
    }

    // -------------------------------------------------------------------------

    private function _clear_setting($settingCode)
    {
        $sql = "DELETE FROM usersettings 
                WHERE username = '$this->username'
                AND setting = '$settingCode'" ;
        $this->ExecSQL($sql);
    }

    private function _insert_setting_value($settingCode, $value)
    {
        $value_esc = mysql_real_escape_string($value);
        $sql = "INSERT INTO usersettings
                (username, setting, value)
                VALUES ('$this->username', '$settingCode', '$value_esc')" ;
        $this->ExecSQL($sql);
    }

    private function _delete_setting_value($settingCode, $value)
    {
        $value_esc = mysql_real_escape_string($value);
        $sql = "DELETE FROM usersettings
                WHERE username = '$this->username'
                    AND setting = '$settingCode'
                    AND value = '$value_esc'";
        $this->ExecSQL($sql);
    }

    // Set a setting to a specific value. If this is a multi-valued setting
    // this will become a single-valued setting.
    // If $value is NULL, remove the setting.
    public function set_value($settingCode, $value)
    {
        if($this->username == '')
        {
            // If we don't have a username set, don't try to write anything
            // to the database. A likely cause of this would be that the
            // current http-requestor is a "guest", not a logged-in user.
            return;
        }

        if(!$this->_isSettable($settingCode))
        {
            throw new InvalidArgumentException("Error: cannot set '$settingCode'");
        }

        $this->_clear_setting($settingCode);
        unset($this->settings_array[$settingCode]);

        if(!is_null($value))
        {
            $this->add_value($settingCode, $value);
        }
    }

    // If no record exists, return $default.
    // Otherwise return what's in the Value column.
    // Note: if setting is really boolean, this will NOT return True, but 'yes' (a string).
    public function get_value($settingCode, $default = Null)
    {
        $values = $this->get_values($settingCode);

        if(count($values) == 0)
            return $default;

        // Ideally we'd throw an exception if we detect that the number of
        // values is more than 1 -- indeed we did this for a while. The
        // problem is that in rare cases two processes can race clearing and
        // setting values. Instead, we resort back to what we did prior to
        // supporting multiple values, which is to just return one of them.
        // The next time this setting is set with set_value(), it should be
        // reduced back down to 1.

        return $values[0];
    }

    // Load a setting value into the $settings_array data structure
    private function _load_value($setting, $value)
    {
        if(isset($this->settings_array[$setting]))
        {
            $this->settings_array[$setting][] = $value;
        }
        else
        {
            $this->settings_array[$setting] = array($value);
        }
    }

    // Add a value to the setting without changing it, even if it already
    // exists. This allows a setting to have multiple values.
    public function add_value($settingCode, $value)
    {
        $this->_load_value($settingCode, $value);
        $this->_insert_setting_value($settingCode, $value);
    }

    // Remove a setting:value pair
    public function remove_value($settingCode, $value)
    {
        if(in_array($value, $this->settings_array[$settingCode]))
        {
            $this->settings_array[$settingCode] = array_diff(
                $this->settings_array[$settingCode], array($value));

            $this->_delete_setting_value($settingCode, $value);
        }
    }

    // Returns an array of values for the setting
    public function get_values($settingCode)
    {
        if (!array_key_exists($settingCode, $this->settings_array))
            return array();

        return $this->settings_array[$settingCode];
    }

    private function ExecSQL($sql)
    {
        if (!mysql_query($sql))
            throw new RuntimeException(mysql_error());
    }

    // -------------------------------------------------------------------------

    public function settings_count()
    {
        return count($this->settings_array);
    }

    // Get an object for this $username. If such an object has
    // already been created, return it. By using this function only
    // and not the constructor, there will only be one object
    // around for each user, and no problems will arise with
    // setting/getting the same settings at various places.
    // If the name is not set, null is returned.
    //
    // We use assignment-by-reference and return-by-reference (note the ampersands)
    // to ensure that multiple returns for the same $username are references to
    // the originally created object, rather than copies of it.
    // (See the PHP manual under "Assignment Operators" and "References Explained".)
    // Callers should also use assignment-by-reference, e.g.
    //     $settings =& Settings::get_settings($username);
    // to ensure that changes in settings are visible everywhere they should be.
    //
    static function & get_Settings($username)
    {
        static $Settings_for_ = array();
        if (array_key_exists($username, $Settings_for_))
        {
            return $Settings_for_[$username];
        }
        else
        {
            $settings = new Settings($username);
            $Settings_for_[$username] =& $settings;
            return $settings;
        }
    }

    // Return a list of unique usernames that have a specific setting
    // specified (and optionally with a specific value).
    static function get_users_with_setting($setting, $value=NULL)
    {
        $usernames = array();
        $sql = sprintf("
            SELECT DISTINCT username
            FROM usersettings
            WHERE setting = '%s'
        ", mysql_real_escape_string($setting));

        if($value)
        {
            $sql .= sprintf(" AND value='%s'",
                mysql_real_escape_string($value));
        }

        $result = mysql_query($sql);
        while ($row = mysql_fetch_assoc($result))
        {
            $usernames[] = $row['username'];
        }
        mysql_free_result($result);

        return $usernames;
    }
}

// vim: sw=4 ts=4 expandtab