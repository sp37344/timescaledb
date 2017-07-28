\ir include/create_single_db.sql

-- table 1
CREATE TABLE "hitest1"(key real, val varchar(40));

-- insertions
INSERT INTO "hitest1" VALUES(0, 'hi');
INSERT INTO "hitest1" VALUES(1, 'sup');
INSERT INTO "hitest1" VALUES(2, 'hello');
INSERT INTO "hitest1" VALUES(3, 'yo');
INSERT INTO "hitest1" VALUES(4, 'howdy');
INSERT INTO "hitest1" VALUES(5, 'hola');
INSERT INTO "hitest1" VALUES(6, 'ya');
INSERT INTO "hitest1" VALUES(1, 'sup');
INSERT INTO "hitest1" VALUES(2, 'hello');
INSERT INTO "hitest1" VALUES(1, 'sup');

-- table 2
CREATE TABLE "hitest2"(name varchar(30), score integer, qualify boolean);

-- insertions
INSERT INTO "hitest2" VALUES('Tom', 6, TRUE);
INSERT INTO "hitest2" VALUES('Mary', 4, FALSE);
INSERT INTO "hitest2" VALUES('Jaq', 3, FALSE);
INSERT INTO "hitest2" VALUES('Jane', 10, TRUE);

-- standard 2 bucket
SELECT histogram(key, 0, 10, 2) FROM hitest1;
-- standard multi-bucket 
SELECT histogram(key, 0, 10, 5) FROM hitest1;
-- standard extra bucket 
SELECT val, histogram(key, 0, 7, 3) FROM hitest1 GROUP BY val;
-- standard element beneath lb
SELECT histogram(key, 1, 7, 3) FROM hitest1;
-- standard element above ub
SELECT histogram(key, 0, 3, 3) FROM hitest1;
-- standard element beneath and above lb and ub, respectively 
SELECT histogram(key, 1, 3, 2) FROM hitest1;

-- error testing: count > 1
SELECT histogram(key, 1, 3, 1) FROM hitest1;
-- error testing: max > min
SELECT histogram(key, 5, 3, 2) FROM hitest1;

-- standard 2 bucket 
SELECT qualify, histogram(score, 0, 10, 2) FROM hitest2 GROUP BY qualify;
-- standard multi-bucket 
SELECT qualify, histogram(score, 0, 10, 5) FROM hitest2 GROUP BY qualify
