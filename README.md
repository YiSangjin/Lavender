# Lavender
SGA Direct Access from Oracle 11g higher.

-- support oracle version : 11g higher.

-- You should not use memory_target.
SQL> show parameter memory_target

NAME                                 TYPE        VALUE
------------------------------------ ----------- ------------------------------
memory_target                        big integer 0

-- compile and start
$sh lvd.sh
