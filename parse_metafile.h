#ifndef PARSE_METAFILE
#define PARSE_METAFILE

// ����������ļ��л�ȡ��tracker��URL
typedef struct _Announce_list {
	char    announce[128];
	struct _Announce_list  *next;
} Announce_list;

// ��������������ļ���·���ͳ���
typedef struct _Files {
	char    path[256];
	long    length;
	struct _Files *next;
} Files; 


//shared vars
extern unsigned char  *metafile_content; // ���������ļ�������
extern long  filesize;                 // �����ļ��ĳ���

extern int       piece_length;    // ÿ��piece�ĳ���,ͨ��Ϊ256KB��262144�ֽ�
extern char      *pieces; // ����ÿ��pieces�Ĺ�ϣֵ,ÿ����ϣֵΪ20�ֽ�
extern int       pieces_length;    // pieces�������ĳ���

extern int       multi_file;    // ָ���ǵ��ļ����Ƕ��ļ�
extern char      *file_name; // ���ڵ��ļ�,����ļ���;���ڶ��ļ�,���Ŀ¼��
extern long long file_length;    // ��Ŵ������ļ����ܳ���
extern Files     *files_head; // ֻ�Զ��ļ�������Ч,��Ÿ����ļ���·���ͳ��� //��ԭ���ĳ�ͻ�ˣ�

extern unsigned char info_hash[20];    // ����info_hash��ֵ,����tracker��peerʱʹ��
extern unsigned char peer_id[21];      // ����peer_id��ֵ,����peerʱʹ��

extern Announce_list *announce_list_head; // ���ڱ�������tracker��������URL

int read_metafile(char *metafile_name);         // ��ȡ�����ļ�
int find_keyword(char *keyword,long *position); // �������ļ��в���ĳ���ؼ���
int read_announce_list();                       // ��ȡ����tracker�������ĵ�ַ
int add_an_announce(char* url);                 // ��tracker�б����һ��URL

int get_piece_length();       // ��ȡÿ��piece�ĳ���,һ��Ϊ256KB
int get_pieces();             // ��ȡ����piece�Ĺ�ϣֵ

int is_multi_files();         // �ж����ص��ǵ����ļ����Ƕ���ļ�
int get_file_name();          // ��ȡ�ļ��������ڶ��ļ�����ȡ����Ŀ¼��
int get_file_length();        // ��ȡ�������ļ����ܳ���
int get_files_length_path();  // ��ȡ�ļ���·���ͳ��ȣ��Զ��ļ�������Ч

int get_info_hash();          // ��info�ؼ��ʶ�Ӧ��ֵ����info_hash               
int get_peer_id();            // ����peer_id,ÿ��peer����һ��20�ֽڵ�peer_id

// �ͷ�parse_metafile.c�ж�̬������ڴ�
void release_memory_in_parse_metafile();
// ���ñ��ļ��ж���ĺ���,��ɽ��������ļ�
int  parse_metafile(char *metafile);

#endif
