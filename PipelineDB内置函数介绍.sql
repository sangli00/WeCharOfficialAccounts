create table tab(col1 hll,col2 bloom,col3 cmsketch,col4 fss,col5 tdigest);
create stream test(x bigint,y text,z timestamp);


--bloom
create continuous view v_bloom as select bloom_agg(x) ,y,z from test group  by y,z;
insert into test values(1,'a','2018-01-11'::timestamp);
insert into test values(1,'a','2018-01-11'::timestamp);
insert into test values(2,'a','2018-01-11'::timestamp);
select bloom_cardinality(bloom_agg),y,z from v_bloom ;
insert into test values(2,'a','2018-01-11'::timestamp);
select bloom_cardinality(bloom_agg),y,z from v_bloom ;
insert into test values(2,'b','2018-01-11'::timestamp);
select bloom_cardinality(bloom_agg),y,z from v_bloom ;


--hll
create continuous view  v_hll as select x,count(distinct y) as cn from test group by x;
explain(analyze,verbose,buffers,costs,timing)  select * from v_hll;
insert into test values(1,'a','2018-01-11'::timestamp);
insert into test values(2,'a','2018-01-11'::timestamp);
insert into test values(2,'a','2018-01-11'::timestamp);
insert into test values(2,'a','2018-01-11'::timestamp);
insert into test values(1,'b','2018-01-11'::timestamp);
insert into test values(1,'b','2018-01-11'::timestamp);
select * from v_hll;


--cmsketch
create continuous view v_cmsketch as select x,y,cmsketch_agg(z) from test group by x,y;
insert into test values(1,'a','2018-01-11'::timestamp);
insert into test values(1,'a','2018-01-12'::timestamp);
select x,y,cmsketch_total(cmsketch_agg) from v_cmsketch;


--FSS
create continuous view v_fss as select fss_agg(x,5),y,z from test group by y,z;
insert into test values(1,'a','2018-01-11'::timestamp);
insert into test values(2,'a','2018-01-11'::timestamp);
insert into test values(3,'a','2018-01-11'::timestamp);
insert into test values(4,'a','2018-01-11'::timestamp);
insert into test values(5,'a','2018-01-11'::timestamp);
insert into test values(6,'a','2018-01-11'::timestamp);

SELECT fss_topk(fss_agg) FROM v_fss;
SELECT fss_topk_values(fss_agg) FROM v_fss;
SELECT fss_topk_freqs(fss_agg) FROM v_fss;

insert into test values(5,'a','2018-01-11'::timestamp);
SELECT fss_topk(fss_agg) FROM v_fss;
SELECT fss_topk_values(fss_agg) FROM v_fss;
SELECT fss_topk_freqs(fss_agg) FROM v_fss;

insert into test values(6,'a','2018-01-11'::timestamp);
SELECT fss_topk(fss_agg) FROM v_fss;
SELECT fss_topk_values(fss_agg) FROM v_fss;
SELECT fss_topk_freqs(fss_agg) FROM v_fss;


--T-Digest
create continuous view v_tdigest as select tdigest_agg(x) ,y,z from test group by y,z;
insert into test values(1,'a','2018-01-11'::timestamp);
insert into test values(2,'a','2018-01-11'::timestamp);
insert into test values(3,'a','2018-01-11'::timestamp);

select tdigest_quantile(tdigest_agg,0.5) ,y,z from v_tdigest;
select tdigest_quantile(tdigest_agg,0.6) ,y,z from v_tdigest;
select tdigest_quantile(tdigest_agg,0.9) ,y,z from v_tdigest;
select tdigest_quantile(tdigest_agg,0.99) ,y,z from v_tdigest;

insert into test values(100,'a','2018-01-11'::timestamp);
select tdigest_quantile(tdigest_agg,0.99) ,y,z from v_tdigest;
