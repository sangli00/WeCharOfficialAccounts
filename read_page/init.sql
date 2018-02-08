create table t(x bigint,y text);
insert into t values(1,'hi postgresql');

insert into t select generate(1,400),'abcdefg';
checkpoint;
