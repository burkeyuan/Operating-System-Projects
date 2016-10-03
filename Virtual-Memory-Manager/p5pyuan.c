#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

/*The parameters can be changed to handle different situations*/
#define Num_frame 8
#define PageSize 256
#define Num_TLB 4
#define Num_page 16

/*Define the struct of Addr
  LogAddr: logical address
  PhyAddr: physical address
  PageNum: page number got from the logical address
  Offset:  offset got from the logical address
  Value:   data got from BACKING_STORE.bin*/ 
struct Addr
{
    int LogAddr;
    int PhyAddr;
    int PageNum;
    int Offset;
    int Value;
};

/*Define the struct of TLBs
  PageNum: page number
  FrameNum:frame number
  Usetime: time when the TLB is most recently used*/
struct TLBs
{
    int PageNum;
    int FrameNum;
    int Usetime;
};

/*Define the struct of PageTables
  FrameNum: frame number for a page
  Usetime:  time when the page is most recently used*/
struct PageTables
{
    int FrameNum;
    int Usetime;
};

/*Set structs and variables
  CurrAddr: stores information of an address 
  TLB[Num_TLBs]: stores information of TLB
  PageTable[Num_page]: stores information of page table
  PhyMem[Num_frame][Pagesize]: physical space and stores values
  frame: used when allocating frames to pages
  tlb: used when looking for the least recently used TLB
  time:virtual time used for recording the use time of TLB and pages
  */
struct Addr CurrAddr = {0x00000000, 0, 0, 0, 0};
struct TLBs TLB[Num_TLB];
struct PageTables PageTable[Num_page];

int Frame[Num_frame];
int PhyMem[Num_frame][PageSize];
int Num_PageFault = 0;
int Num_TLBHit = 0;
int frame = 0;
int tlb = 0;
int time1 = 0;

/*Initial all elements in TLB to be -1 or 0*/
void init_TLB()
{
    int i = 0;
    for(i = 0; i < Num_TLB; i++)
    {
        TLB[i].PageNum = -1;
        TLB[i].FrameNum = -1;
        TLB[i].Usetime = 0;
    }
}
/*Initial all elements in page table to be -1 or 0*/
void init_PageTable()
{
    int i = 0;
    for(i = 0; i < Num_page; i++)
    {
        PageTable[i].FrameNum = -1;
        PageTable[i].Usetime = 0;
    }
}

/*Get page number and offset from a logical address*/
void extract()
{
    CurrAddr.PageNum = (CurrAddr.LogAddr & 0x0000FF00) >> (8);
    CurrAddr.Offset = CurrAddr.LogAddr & 0x000000FF;
}

/*Get value from corresponding physical address to a logical address*/
void getvalue()
{
    CurrAddr.Value = PhyMem[PageTable[CurrAddr.PageNum].FrameNum][CurrAddr.Offset];
}

/*Copy the data from BACKING_STORE.bin to physical space*/
void Move()
{
    char temp[256];
    int i;
    
    FILE *pBS;
    pBS = fopen("BACKING_STORE.bin", "rb");
    
    if(pBS == NULL)
    {
        printf("Error: cannot open BACKING_STORE.bin");
    }
    
    if(fseek(pBS, CurrAddr.PageNum * PageSize, SEEK_SET) != 0)
    {
        printf("Error: fseek failed");
    }

    if(fread(temp, 1, PageSize, pBS) != PageSize)
    {
        printf("Error: fread failed");
    }
    
    fclose(pBS);
    for(i = 0; i < PageSize; i++)
    {
        PhyMem[PageTable[CurrAddr.PageNum].FrameNum][i] = temp[i];
    }
}

/*Scan page table to check whether a page is in the page table or not*/
void PageTable_Consult()
{
    int min = 65535;   
    int i, j;

/*If the page is already in table, increase time1 by 1, update the use time of the page and calculate the physical address*/  
    if(PageTable[CurrAddr.PageNum].FrameNum > -1)
    { 
        printf("page %i is contained in frame %i\n", CurrAddr.PageNum, PageTable[CurrAddr.PageNum].FrameNum);     
        PageTable[CurrAddr.PageNum].Usetime = time1;
        getvalue();
        CurrAddr.PhyAddr = PageTable[CurrAddr.PageNum].FrameNum * PageSize + CurrAddr.Offset;
    }   

/*If not and 'frame' is less than the number of frames, set the frame number of related page to 'frame' and do operations like above*/
    if(PageTable[CurrAddr.PageNum].FrameNum == -1 && frame < Num_frame)
    {
        PageTable[CurrAddr.PageNum].FrameNum = frame;
        Frame[frame] = CurrAddr.PageNum;
        printf("virtual address %i contained in page %i causes a page fault\n", CurrAddr.LogAddr, CurrAddr.PageNum);
        Move();
        printf("page %i is loaded into frame %d\n", CurrAddr.PageNum, PageTable[CurrAddr.PageNum].FrameNum);
        getvalue();
        frame++;
        Num_PageFault++;
        PageTable[CurrAddr.PageNum].Usetime = time1;
        CurrAddr.PhyAddr = PageTable[CurrAddr.PageNum].FrameNum * PageSize + CurrAddr.Offset;
    }

/*If not and 'frame' is bigger than the number of frames minus 1, use LRU algorithm to look for the least recently used page and
  corresponding frame, then allocate the frame to the new page and do operations like above*/  
    if(PageTable[CurrAddr.PageNum].FrameNum == -1 && frame > (Num_frame - 1))
    {    
         printf("virtual address %i contained in page %i causes a page fault\n", CurrAddr.LogAddr, CurrAddr.PageNum);
         for(i = 0; i < Num_page; i++)
         {
             if(PageTable[i].FrameNum != -1)
             {
                 if(PageTable[i].Usetime < min)
                 {
                     min = PageTable[i].Usetime;
                     j = i;
                 }
             }
         }
         PageTable[CurrAddr.PageNum].FrameNum = PageTable[j].FrameNum;
         Frame[PageTable[j].FrameNum] = CurrAddr.PageNum;
         PageTable[j].FrameNum = -1;
         Move();
         printf("page %i is loaded into frame %d\n", CurrAddr.PageNum, PageTable[CurrAddr.PageNum].FrameNum);
         getvalue();
         Num_PageFault++;
         PageTable[CurrAddr.PageNum].Usetime = time1;
         CurrAddr.PhyAddr = PageTable[CurrAddr.PageNum].FrameNum * PageSize + CurrAddr.Offset;
    }
}

/*Scan TLB to check whether a page number is in it or not*/
void TLB_Consult()
{
    int i = 0;
    int flag_exist = 0;

/*If so, increase number of TLB hits by 1 and update the information of both TLB and page table*/
    for(i = 0; i < Num_TLB; i++)
    {
	if(TLB[i].PageNum == CurrAddr.PageNum)
	{
            flag_exist = 1;
            Num_TLBHit++;
            //printf("page %i is contained in frame %i\n", CurrAddr.PageNum, PageTable[CurrAddr.PageNum].FrameNum);  
            TLB[i].Usetime = time1;
            PageTable[CurrAddr.PageNum].Usetime = time1;
            printf("page %i is stored in frame %i which is stored in entry %d of the TLB\n", TLB[i].PageNum, TLB[i].FrameNum, i);
            CurrAddr.PhyAddr = TLB[i].FrameNum * PageSize + CurrAddr.Offset;
            getvalue();
            break; 
        }
    }

/*If not, execute PageTable_Consult() firstly and then search for the least recently used TLB and repalce it with new page*/
    if(flag_exist == 0)
    {
        printf("frame number for page %i is missing in the TLB\n", CurrAddr.PageNum);
        PageTable_Consult();
        int min = 65535;
        for(i = 0; i < Num_TLB; i++)
        {
            if(TLB[i].Usetime < min)
            {
                min = TLB[i].Usetime;
                tlb = i;
            }
        }
        TLB[tlb].PageNum = CurrAddr.PageNum;
        TLB[tlb].FrameNum = PageTable[CurrAddr.PageNum].FrameNum;
        printf("page %i is stored in frame %i which is stored in entry %d of the TLB\n", TLB[i].PageNum, TLB[i].FrameNum, i);
        TLB[tlb].Usetime = time1;
    }
}

/*main() function*/
int main()
{
    int j = 0, i = 0;
    int num = 0;
    int read[1000];
    char address[32];
    FILE *p;

    init_TLB();
    init_PageTable();
   
    if((p = fopen("addresses.txt","r")) == NULL)
    {
        printf("cannot open file\n");
        exit(0);
    }
    
    while(1)
    {
        fgets(address, 32, p);
        if(feof(p))
        {
             break;
        }
        read[i] = atoi(address);
        i++;
    }
    fclose(p);
    num = i;
    for(i = 0; i < num; i++){
        time1++;
        CurrAddr.LogAddr = read[i];
        extract();
        TLB_Consult();
        CurrAddr.LogAddr = 0;
    }
    
    for(j = 0; j < Num_page; j++)
    {
        if(PageTable[j].FrameNum != -1){
            printf("page %d: frame %i\n", j, PageTable[j].FrameNum);
        }else{
            printf("page %d: not in memory\n", j);
        }
    }
 
    for(j = 0; j < Num_frame; j++)
    {
        printf("frame %d: page %i\n", j, Frame[j]);
    }   

    printf("%d page faults out of %d references\n", Num_PageFault, num);
    printf("%d hits out of %d references\n", Num_TLBHit, num);
    return 0;
}



