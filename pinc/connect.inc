<?php

class dbConnect
{
    var $error='';

    function dbConnect() {
        include('udb_user.php');

        // Ignore warnings from mysql_connect() so we can handle them
        // gracefully elsewhere.
        $this->db_lk=@mysql_connect($db_server,$db_user,$db_password);
        if (!$this->db_lk) {
            $this->error= "Unable to connect to database server."; return 0;
        }

        if (!mysql_select_db($db_name,$this->db_lk)) {
            $this->error= "Unable to locate database."; return 0;
        }
        else {
            return 1;
        }
    }

    function close() {
        if (isset($this->db_lk)) {
            mysql_close($this->db_lk);
        }
    }
}

// vim: sw=4 ts=4 expandtab