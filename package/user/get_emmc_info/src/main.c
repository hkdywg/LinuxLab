#include "main.h"
#include "extra_mmc.h"

/*********************************************************************
 *function name :   print_usage
 *Descriptions  :   Print Instructions menu
 ********************************************************************/
int print_usage(){
    
    int ret = 0;
    
    printf("usage: emmc_get_info <path_space> -f [option]\r\n");
    printf("\r\n");
    printf("-[c] get extcsd message from eMMC\r\n");
    printf("-[h] display the help menu\r\n");
    printf("-[bw]   get bw eMMC life message\r\n"); 
    printf("-[jbl]  get jbl life eMMC life message\r\n");
    printf("-[ky]   get ky eMMC life message\r\n");
    printf("\r\n");
    
    return ret = 1;
}

/*********************************************************************
 *function name :   main
 ********************************************************************/
int main(int argc,char *argv[]){
    
    int ret = 0;
    
    if((4 == argc)&&(1 == strlen(argv[3]))){
        switch(*argv[3]){
            case 'c' :ret = get_emmc_extcsd(argv);break;
            
            case 'h' :print_usage();break;
            default:break;
        }
        return ret;
    }else if(4 == argc){
        if(strcmp("bw",argv[3])==0){
            ret = do_general_cmd_read(argv,type_bw);
            ret = get_emmc_extinfo(argv,type_bw);
        }if(strcmp("jbl",argv[3])==0){
            ret = do_general_cmd_read(argv,type_jbl);
            ret = get_emmc_extinfo(argv,type_jbl);
        }if(strcmp("ky",argv[3])==0){
            ret = do_general_cmd_read(argv,type_ky);
            ret = get_emmc_extinfo(argv,type_ky);
        }
    }else{
        print_usage();
    }
    
    return ret;
}
