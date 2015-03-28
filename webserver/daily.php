<?php
//    Smart Meter data aggregation program, php database interface
//    Copyright (C) 2014-2015 Hans de Rijck
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
$startDate = htmlspecialchars($_GET["fromdate"]);
$endDate = htmlspecialchars($_GET["todate"]);

$numPixels=1000;

$sampleRate = 6 * 60 * 24;

$file_db = new PDO('sqlite:meetdata.sqlite');
if($file_db){

    $encodable = array();

    $incr = floor((($endDate-$startDate)/$sampleRate)/$numPixels);

    $cnt = 0;

    // Write query result to file
    $output = fopen( "/var/www/file.csv", "w");
    fprintf( $output, "datum,electra,gas,temp\n" );

    fclose($output);


shell_exec  ( "mysql -uUSER -pPASSWORD meetdata -N -e \"SELECT CONCAT_WS(',',datum,electra_low_day + electra_high_day,gas_day,t_avg) FROM daily;\" >> /var/www/file.csv  ") ;

    // Send this file to the browser
    header("Content-type: text/csv");
    header("Content-Disposition: attachment; filename=file.csv");
    header("Pragma: no-cache");
    header("Expires: 0");

    ob_clean();
    flush();
    readfile("/var/www/file.csv" );

}else{
    die("unable to connect to db");
}
?>
