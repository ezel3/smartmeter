# smartmeter
Smart energy meter readout

This program runs on a Raspberry PI and periodically polls a 'smart' energy meter (both electricity and gas).
These meters are becoming very common in Dutch households, and they all have a unique feature: they can be read out by the owner.

This program polls the electricity meter every 10 seconds and stores the result in a MYSQL database.

From this database, it is possible to run fancy graphs (see examples below).

At the moment, the program is a bit of a mess, because it is being developed in VIM.

I hope to clean up this mess by using Visual Studio with the wonderful VisualGDB (http://www.visualgdb.com/) plugin.

License: This program is released under the GNU General Public License (GPL)

![daily](https://cloud.githubusercontent.com/assets/6112759/6871079/715fa6b4-d4a0-11e4-8619-205665dcc859.PNG)

![yearly](https://cloud.githubusercontent.com/assets/6112759/6871055/4749ae24-d4a0-11e4-9ed3-cba6bc859e3f.PNG)