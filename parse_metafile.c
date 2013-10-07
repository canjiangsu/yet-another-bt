#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "parse_metafile.h"
#include "bt_hash.h"

unsigned char  *metafile_content = NULL; // ���������ļ�������
long  filesize;                 // �����ļ��ĳ���

int       piece_length  = 0;    // ÿ��piece�ĳ���,ͨ��Ϊ256KB��262144�ֽ�
char      *pieces       = NULL; // ����ÿ��pieces�Ĺ�ϣֵ,ÿ����ϣֵΪ20�ֽ�
int       pieces_length = 0;    // pieces�������ĳ���

int       multi_file    = 0;    // ָ���ǵ��ļ����Ƕ��ļ�
char      *file_name    = NULL; // ���ڵ��ļ�,����ļ���;���ڶ��ļ�,���Ŀ¼��
long long file_length   = 0;    // ��Ŵ������ļ����ܳ���
Files     *files_head   = NULL; // ֻ�Զ��ļ�������Ч,��Ÿ����ļ���·���ͳ��� //��ԭ���ĳ�ͻ�ˣ�

unsigned char info_hash[20];    // ����info_hash��ֵ,����tracker��peerʱʹ��
unsigned char peer_id[21];      // ����peer_id��ֵ,����peerʱʹ��

Announce_list *announce_list_head = NULL; // ���ڱ�������tracker��������URL


int read_metafile(char *metafile_name)
{
	long  i;
	
	// �Զ����ơ�ֻ���ķ�ʽ���ļ�
	FILE *fp = fopen(metafile_name,"rb");
	if(fp == NULL) {
		printf("%s:%d can not open file\n",__FILE__,__LINE__);
		return -1;
	}

	// ��ȡ�����ļ��ĳ���
	fseek(fp,0,SEEK_END);
	filesize = ftell(fp);
	if(filesize == -1) {
		printf("%s:%d fseek failed\n",__FILE__,__LINE__);
		return -1;
	}
	
	metafile_content = (char *)malloc(filesize+1);
	if(metafile_content == NULL) {
		printf("%s:%d malloc failed\n",__FILE__,__LINE__);
		return -1;
	}
	
	// ��ȡ�����ļ������ݵ�metafile_content��������
	fseek(fp,0,SEEK_SET);
	for(i = 0; i < filesize; i++)
		metafile_content[i] = fgetc(fp);
	metafile_content[i] = '\0';

	fclose(fp); 

#ifdef DEBUG
	printf("metafile size is: %ld\n",filesize);
#endif	
	
	return 0;
}

int find_keyword(char *keyword,long *position)
{
	long i;

	*position = -1;
	if(keyword == NULL)  return 0;

	for(i = 0; i < filesize-strlen(keyword); i++) { //compare per i increase
		if( memcmp(&metafile_content[i], keyword, strlen(keyword)) == 0 ) {
			*position = i;
			return 1;
		}
	}
	
	return 0;
}

int read_announce_list()
{
	Announce_list  *node = NULL;
	Announce_list  *p    = NULL;
	int            len   = 0;
	long           i;

	if( find_keyword("13:announce-list",&i) == 0 ) {
        /*printf("never expected to enter here?");*/
		if( find_keyword("8:announce",&i) == 1 ) {
			i = i + strlen("8:announce");
			while( isdigit(metafile_content[i]) ) {
				len = len * 10 + (metafile_content[i] - '0');
				i++;
			}
			i++;  // ���� ':'

			node = (Announce_list *)malloc(sizeof(Announce_list));
			strncpy(node->announce,&metafile_content[i],len);
			node->announce[len] = '\0';
			node->next = NULL;
			announce_list_head = node;
		}
	} 
	else {  // �����13:announce-list�ؼ��ʾͲ��ô���8:announce�ؼ��� 
		i = i + strlen("13:announce-list");
		i++;         // skip 'l'
		while(metafile_content[i] != 'e') {
			i++;     // skip 'l'
			while( isdigit(metafile_content[i]) ) {
				len = len * 10 + (metafile_content[i] - '0');
				i++;
			}
			if( metafile_content[i] == ':' )  i++;
			else  return -1;

			// ֻ������http��ͷ��tracker��ַ,��������udp��ͷ�ĵ�ַ
			if( memcmp(&metafile_content[i],"http",4) == 0 ) {
				node = (Announce_list *)malloc(sizeof(Announce_list));
				strncpy(node->announce,&metafile_content[i],len);
				node->announce[len] = '\0';
				node->next = NULL;

				if(announce_list_head == NULL)
					announce_list_head = node;
				else {
					p = announce_list_head;
					while( p->next != NULL) p = p->next; // ʹpָ���������
					p->next = node; // node��Ϊtracker�б�����һ�����
				}
			}

			i = i + len;
			len = 0;
			i++;    // skip 'e'
			if(i >= filesize)  return -1;
		}	
	}

#ifdef DEBUG
	p = announce_list_head;
	while(p != NULL) {
		printf("%s\n",p->announce);
		p = p->next;
	}
#endif	
	
	return 0;
}

// ����ĳЩtrackerʱ�᷵��һ���ض���URL,��Ҫ���Ӹ�URL���ܻ�ȡpeer
int add_an_announce(char *url)
{
	Announce_list *p = announce_list_head, *q;

	// ������ָ����URL��tracker�б����Ѵ���,���������
	while(p != NULL) {
		if(strcmp(p->announce,url) == 0)  break;
		p = p->next;
	}
	if(p != NULL)  return 0;

	q = (Announce_list *)malloc(sizeof(Announce_list));
	strcpy(q->announce,url);
	q->next = NULL;
	
	p = announce_list_head;
	if(p == NULL)  { announce_list_head = q; return 1; }
	while(p->next != NULL)  p = p->next;
	p->next = q;
	return 1;
}

int is_multi_files()
{
	long i;

	if( find_keyword("5:files",&i) == 1 ) {
    
		multi_file = 1;
        
        #ifdef DEBUG
            printf("is_multi_files:%d\n",multi_file);
        #endif

		return 1;
	}

	return 0;
}

int get_piece_length()
{
	long i;

	if( find_keyword("12:piece length",&i) == 1 ) {
		i = i + strlen("12:piece length");  // skip "12:piece length"
		i++;  // skip 'i'
		while(metafile_content[i] != 'e') {
			piece_length = piece_length * 10 + (metafile_content[i] - '0');
			i++;
		}
	} else {
		return -1;
	}

#ifdef DEBUG
	printf("per piece length:%d\n",piece_length);
#endif

	return 0;
}

int get_pieces() //�õ�ȫ����Ƭ��hashֵ
{
	long i;

	if( find_keyword("6:pieces", &i) == 1 ) {
		i = i + 8;     // skip "6:pieces"
		while(metafile_content[i] != ':') {
			pieces_length = pieces_length * 10 + (metafile_content[i] - '0');
			i++;
		}
		i++;           // skip ':'
		pieces = (char *)malloc(pieces_length+1);
		memcpy(pieces,&metafile_content[i],pieces_length);
		pieces[pieces_length] = '\0';
	} else {
		return -1;
	}

#ifdef DEBUG
	printf("get_pieces ok\n");
	printf("pieces_hash_buffer_length is %d\n",pieces_length);
    /*printf("pieces_hash_content is %s\n",pieces);*/
#endif

	return 0;
}

int get_file_name()
{
	long  i;
	int   count = 0;

	if( find_keyword("4:name", &i) == 1 ) {
		i = i + 6;  // skip "4:name"
		while(metafile_content[i] != ':') {
			count = count * 10 + (metafile_content[i] - '0');
			i++;
		}
		i++;        // skip ':' 
		file_name = (char *)malloc(count+1);
		memcpy(file_name,&metafile_content[i],count);
		file_name[count] = '\0';
	} else {
		return -1;
	}

#ifdef DEBUG
	// ���ڿ��ܺ��������ַ�,��˿��ܴ�ӡ������
     printf("file_name:%s\n",file_name);
#endif

	return 0;
}

int get_file_length()
{
	long i;

	if(is_multi_files() == 1)  {
		if(files_head == NULL)  get_files_length_path();
		Files *p = files_head;
		while(p != NULL) { file_length += p->length; p = p->next; }
	} else {
		if( find_keyword("6:length",&i) == 1 ) {
			i = i + 8;  // skip "6:length"
			i++;        // skip 'i' 
			while(metafile_content[i] != 'e') {
				file_length = file_length * 10 + (metafile_content[i] - '0');
				i++;
			}	
		}
	}
	
#ifdef DEBUG
	printf("file_length:%lld\n",file_length);
#endif

	return 0;
}

int get_files_length_path()
{
	long   i;
	int    length;
	int    count;
	Files  *node  = NULL;
	Files  *p     = NULL;

	if(is_multi_files() != 1) {
		return 0;
	}
	
	for(i = 0; i < filesize-8; i++) {
		if( memcmp(&metafile_content[i],"6:length",8) == 0 )
		{
			i = i + 8;  // skip "6:length"
			i++;        // skip 'i' 
			length = 0;
			while(metafile_content[i] != 'e') {
				length = length * 10 + (metafile_content[i] - '0');
				i++;
			}
			node = (Files *)malloc(sizeof(Files));
			node->length = length;
			node->next = NULL;
			if(files_head == NULL)
				files_head = node;
			else {
				p = files_head;
				while(p->next != NULL) p = p->next;
				p->next = node;
			}
		}
		if( memcmp(&metafile_content[i],"4:path",6) == 0 )
		{
			i = i + 6;  // skip "4:path"
			i++;        // skip 'l'
			count = 0;
			while(metafile_content[i] != ':') {
				count = count * 10 + (metafile_content[i] - '0');
				i++;
			}
			i++;        // skip ':'
			p = files_head;
			while(p->next != NULL) p = p->next;
			memcpy(p->path,&metafile_content[i],count);
			*(p->path + count) = '\0';
		}
	}

#ifdef DEBUG
	// ���ڿ��ܺ��������ַ�,��˿��ܴ�ӡ������
     p = files_head;
     while(p != NULL) {
         printf("%ld:%s\n",p->length,p->path);
         p = p->next;
     }
#endif

	return 0;
}

int get_info_hash()
{
    int   push_pop = 0;
    long  i, begin, end;

    if(metafile_content == NULL)  return -1;

    if( find_keyword("4:info",&i) == 1 ) {
        begin = i+6;  // begin�ǹؼ���"4:info"��Ӧֵ����ʼ�±�
    } else {
        return -1;
    }

    i = i + 6;        // skip "4:info"
    for(; i < filesize; ) //check the whole file
        if(metafile_content[i] == 'd') { 
            push_pop++;
            i++;
        } else if(metafile_content[i] == 'l') {
            push_pop++;
            i++;
        } else if(metafile_content[i] == 'i') {
            i++;  // skip i
            if(i == filesize)  return -1;
            while(metafile_content[i] != 'e') {
                if((i+1) == filesize)  return -1;
                else i++;
            }
            i++;  // skip e
        } else if((metafile_content[i] >= '0') && (metafile_content[i] <= '9')) {
            int number = 0;
            while((metafile_content[i] >= '0') && (metafile_content[i] <= '9')) {
                number = number * 10 + metafile_content[i] - '0';
                i++;
            }
            i++;  // skip :
            i = i + number;
        } else if(metafile_content[i] == 'e') {
            push_pop--;
            if(push_pop == 0) { end = i; break; }
            else  i++; 
        } else {
            return -1;
        }
    if(i == filesize)  return -1;

    bt_hash_t *hash = bt_hash_new();
    bt_hash_update(hash,&metafile_content[begin],end-begin+1);
    bt_hash_data(hash);
    
    //TODO change to OO 
    for(i = 0; i < 20; i++)  
    {
        info_hash[i] = hash->hash[i];
    }

    bt_hash_destroy(&hash);

#ifdef DEBUG
    printf("info_hash:");
    for(i = 0; i < 20; i++)  
    {
        printf("%.2x ",info_hash[i]);   
    }

    printf("\n");

#endif

    return 0;
}

int get_peer_id()
{

    // ���ò��������������
    srand(time(NULL));
    // ���������,��������12λ����peer_id,peer_idǰ8λ�̶�Ϊ-TT1000-
    sprintf((char *)peer_id,"-TT1000-%12d",rand());

#ifdef DEBUG
    int i;
    printf("peer_id:");
    for(i = 0; i < 20; i++)  printf("%c",peer_id[i]);
    printf("\n");
#endif

    return 0;
}

void release_memory_in_parse_metafile()
{
    Announce_list *p;
    Files         *q;

    if(metafile_content != NULL)  free(metafile_content);
    if(file_name != NULL)         free(file_name);
    if(pieces != NULL)            free(pieces);

    while(announce_list_head != NULL) {
        p = announce_list_head;
        announce_list_head = announce_list_head->next;
        free(p);
    }

    while(files_head != NULL) {
        q = files_head;
        files_head = files_head->next;
        free(q);
    }
}

int parse_metafile(char *metafile)
{
    int ret;

    // ��ȡ�����ļ�
    ret = read_metafile(metafile);
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // �������ļ��л�ȡtracker�������ĵ�ַ
    ret = read_announce_list();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // �ж��Ƿ�Ϊ���ļ�
    ret = is_multi_files();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // ��ȡÿ��piece�ĳ���,һ��Ϊ256KB
    ret = get_piece_length();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // ��ȡ����piece�Ĺ�ϣֵ
    ret = get_pieces();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // ��ȡҪ���ص��ļ��������ڶ��ļ������ӣ���ȡ����Ŀ¼��
    ret = get_file_name();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // ���ڶ��ļ������ӣ���ȡ���������ص��ļ�·�����ļ�����
    ret = get_files_length_path();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // ��ȡ�����ص��ļ����ܳ���
    ret = get_file_length();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    // ���info_hash������peer_id
    ret = get_info_hash();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    printf("in parse_metafile end multi_file:%d\n",multi_file);
    ret = get_peer_id();
    if(ret < 0) { printf("%s:%d wrong",__FILE__,__LINE__); return -1; }

    return 0;
}
