#include "shellpp_ii_module.h"
typedef void *(*find_t)(unsigned short); typedef int (*add_t)(unsigned short); typedef void *(*head_t)(void *); typedef void *(*next_t)(void *,void *); typedef void (*remove_t)(void *,void *); typedef void *(*label_create_t)(void *); typedef void (*label_set_t)(void *,const char *); typedef void (*obj_pos_t)(void *,int,int); typedef void (*title_t)(void *,const char *,int,int,int);
#define FIND_ID ((find_t)0x010414C1)
#define FIND_PAGE ((find_t)0x010416A9)
#define LAUNCH_ADD ((add_t)0x00EFBC81)
#define LL_HEAD ((head_t)0x009313F5)
#define LL_NEXT ((next_t)0x00931415)
#define LL_REMOVE ((remove_t)0x009312F5)
#define LABEL_CREATE ((label_create_t)0x009410B9)
#define LABEL_SET ((label_set_t)0x00941B1D)
#define OBJ_POS ((obj_pos_t)0x00917895)
#define TITLE_CREATE ((title_t)0x00ED1B81)
#define APP_ID 0x79u
#define PAGE_ID 0x00790000u
#define TABLE_ADDR 0x01571640u
#define TABLE_SLOT 0x4021AC00u
#define AM_HEAD 0x4021AC2Cu
#define LAUNCHER_LIST 0x4020D6A0u
static unsigned char table[256], desc[128], info[132], pixel[4]={0x20,0xA8,0xF2,0xFF};
typedef struct { unsigned int h,wh,stride,size; const unsigned char *data; } icon_t; static icon_t icon; static volatile unsigned int registered,published; static const char package_name[]="com.shellpp.ii",display_name[]="Shell++ II",page_name[]="shellpp-ii";
static const char *app_name(void *x){(void)x;return display_name;} static void signal_cb(void *a,void *b){(void)a;(void)b;}
static int create_cb(void *page,void *root){void *l;(void)page;if(!root)return -1;TITLE_CREATE(root,display_name,0,0,0);l=LABEL_CREATE(root);if(l){OBJ_POS(l,28,92);LABEL_SET(l,"Shell++ II is running");}return 0;}
static void *find_id(unsigned short id){return id==APP_ID?(void *)info:FIND_ID(id);} static void *find_page(unsigned short id){return id==APP_ID?(void *)desc:FIND_PAGE(id);}
static void zero(void *p,unsigned int n){unsigned int i;for(i=0;i<n;i++)((unsigned char*)p)[i]=0;}
static int eq(unsigned int p){unsigned int i=0;if(!p)return 0;while(package_name[i]){if(*(volatile unsigned char*)(p+i)!=(unsigned char)package_name[i])return 0;i++;}return *(volatile unsigned char*)(p+i)==0;}
static int has_launcher(void){void *n=LL_HEAD((void*)LAUNCHER_LIST);while(n){if(eq(*(volatile unsigned int*)((unsigned int)n+8)))return 1;n=LL_NEXT((void*)LAUNCHER_LIST,n);}return 0;}
static void insert_page(void){unsigned int h,f;zero(desc,sizeof(desc));*(unsigned int*)(desc+0)=0x4021ACAC;*(unsigned int*)(desc+0x10)=(unsigned int)page_name;*(unsigned int*)(desc+0x14)=PAGE_ID;*(unsigned int*)(desc+0x18)=6;*(unsigned int*)(desc+0x28)=0x02000504;*(unsigned int*)(desc+0x2c)=AM_HEAD;*(unsigned int*)(desc+0x34)=(unsigned int)signal_cb;*(unsigned int*)(desc+0x48)=0x4021ACAC;*(unsigned int*)(desc+0x4c)=(unsigned int)create_cb;*(unsigned int*)(desc+0x50)=(unsigned int)signal_cb;*(unsigned int*)(desc+0x5c)=(unsigned int)signal_cb;*(unsigned int*)(desc+0x64)=(unsigned int)signal_cb;*(unsigned int*)(desc+0x74)=0x42AC0000;*(unsigned int*)(desc+0x78)=0x42940000;*(unsigned int*)(desc+0x7c)=0x42AC0000;h=*(volatile unsigned int*)AM_HEAD;if(!h)return;f=*(volatile unsigned int*)(h+68);*(unsigned int*)(desc+64)=h;*(unsigned int*)(desc+68)=f;*(volatile unsigned int*)(h+68)=(unsigned int)desc;if(f)*(volatile unsigned int*)(f+64)=(unsigned int)desc;}
int shellpp_app_register(void){unsigned int i;volatile unsigned char *src;if(registered)return 0;icon.h=0x919;icon.wh=0x00010001;icon.stride=0x00040000;icon.size=4;icon.data=pixel;zero(info,sizeof(info));*(unsigned int*)(info+8)=(unsigned int)package_name;*(unsigned int*)(info+12)=(unsigned int)&icon;*(unsigned short*)(info+16)=APP_ID;*(unsigned int*)(info+28)=(unsigned int)app_name;src=(volatile unsigned char*)TABLE_ADDR;for(i=0;i<sizeof(table);i++)table[i]=src[i];*(unsigned int*)(table+4)=(unsigned int)find_id;*(unsigned int*)(table+76)=(unsigned int)find_page;*(volatile unsigned int*)TABLE_SLOT=(unsigned int)table;insert_page();registered=1;return 0;}
int shellpp_app_publish(void){if(!registered)return -1;if(!has_launcher()&&LAUNCH_ADD(APP_ID)!=0)return -1;published=1;return 0;}
int shellpp_app_remove(void){unsigned int p,n;void *x;if(published){x=LL_HEAD((void*)LAUNCHER_LIST);while(x){if(eq(*(volatile unsigned int*)((unsigned int)x+8))){LL_REMOVE((void*)LAUNCHER_LIST,x);break;}x=LL_NEXT((void*)LAUNCHER_LIST,x);}}if(registered){*(volatile unsigned int*)TABLE_SLOT=TABLE_ADDR;p=*(unsigned int*)(desc+64);n=*(unsigned int*)(desc+68);if(p)*(volatile unsigned int*)(p+68)=n;if(n)*(volatile unsigned int*)(n+64)=p;}registered=0;published=0;return 0;}
