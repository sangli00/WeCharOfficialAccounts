#include "read_page.h"
extern char *text_to_cstring(const text *t);


int getPageNum(int fd){
    struct stat statbuf;

    if(fd <0)
        return -1;

    if(fstat(fd,&statbuf) == -1)
        return -1;
        
    return (statbuf.st_size)/BLOCK_SIZE;
}

struct varlena *
heap_tuple_untoast_attr(struct varlena *attr)
{
	/*toast...*/
	return NULL;
}

struct varlena *
pg_detoast_datum_packed(struct varlena *datum)
{
    if (VARATT_IS_COMPRESSED(datum) || VARATT_IS_EXTERNAL(datum))
        return heap_tuple_untoast_attr(datum);
    else
        return datum;
}

char *
text_to_cstring(const text *t)
{
    /* must cast away the const, unfortunately */
    text       *tunpacked = pg_detoast_datum_packed((struct varlena *) t);
    int            len = VARSIZE_ANY_EXHDR(tunpacked);
    char       *result;

    result = (char *) malloc(len + 1);
    memcpy(result, VARDATA_ANY(tunpacked), len);
    result[len] = '\0';

    if (tunpacked != t)
        free(tunpacked);

    return result;
}

size_t read_block(int fd,char *page)
{
	size_t s;
	s = read(fd,(void*)page,BLOCK_SIZE);

	return s;
}

int
open_file(char *file)
{
	int fd;

	fd = open(file,O_RDONLY);

	return fd;
}

void
printtup(char *page)
{
	int rows;
	int i;
	int offset;
	ItemId item;
	HeapTupleHeader tuple;
	char *col1 = NULL;

	item = (ItemId)malloc(sizeof(ItemIdData));
	tuple = (HeapTupleHeader)malloc(sizeof(HeapTupleHeaderData));
	col1 = (char*)malloc(sizeof(char)*8);


	rows = PageGetMaxOffsetNumber(page);

	for(i = 0;i<rows;i++)
	{
		int text_offset;
		char *tmp_text;		
		char *result;
		int64 val;

		offset = SizeOfPageHeaderData + i*sizeof(ItemIdData);	
		memcpy(item,page+offset,sizeof(ItemIdData));	
		memcpy(tuple,page+item->lp_off,item->lp_len);

		memcpy(col1, page+item->lp_off+HEAPTUPLESIZE,sizeof(char)*8); 
		val  = *(int64 *)col1;

		text_offset = HEAPTUPLESIZE+8;
		tmp_text = page+item->lp_off+text_offset;
		
		result = text_to_cstring((text *)tmp_text);
		
		printf("%ld,%s\n",*(int64*)col1,result);

		memset(col1,0,sizeof(char)*8);
		memset(item,0,sizeof(ItemIdData));
		memset(tuple,0,sizeof(HeapTupleHeaderData));
	}

}


int
main(int argc,char *argv[])
{
	int fd;
	char *page = NULL;
	char *path = NULL;
	int pages = 0;
	int i = 0;

	if(argc <2)
	{
		printf("%s\n","ERROR: need table file path");
		return 0 ;
	}
		

	path=  argv[1];
	
	if((fd = open_file(path)) == -1)
	{
		printf("%s\n","ERROR: table file cannot open,please check");
		return 0;
	}

	pages = getPageNum(fd);
	
	page = (char*)malloc(BLOCK_SIZE);

	for(i = 0;i<pages;i++)
	{
		size_t size = read_block(fd,page);

		printtup(page);
		memset(page,0,BLOCK_SIZE);
		lseek(fd,0,SEEK_CUR);
	}
	return 0;

}
