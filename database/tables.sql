
 CREATE TABLE `daily` (
  `datum` datetime NOT NULL,
  `electra_low` int(11) DEFAULT NULL,
  `electra_high` int(11) DEFAULT NULL,
  `gas` float DEFAULT NULL,
  `electra_low_day` int(11) DEFAULT NULL,
  `electra_high_day` int(11) DEFAULT NULL,
  `gas_day` float DEFAULT NULL,
  `t_min` float DEFAULT NULL,
  `t_max` float DEFAULT NULL,
  `t_avg` float DEFAULT NULL,
  PRIMARY KEY (`datum`)
)

 CREATE TABLE `electra` (
  `datum` datetime DEFAULT NULL,
  `waarde` int(11) DEFAULT NULL
)
ENGINE=MyISAM DEFAULT CHARSET=latin1