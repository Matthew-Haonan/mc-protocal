#include<stdio.h>
#include<stdlib.h>
#include"modbus.h"

#include"modbus-tcp.h"

int main(void)
{
  modbus_t *mb;
  uint16_t tab_reg[32]={0};

  mb = modbus_new_tcp("192.168.0.22",5800);//由于是tcp client 连接，在同一程序中相同的端口可以连接多次
  modbus_set_debug(mb,1);
  modbus_set_slave(mb,1);//从机地址
  modbus_connect(mb);

  printf("hello\n");

  struct timeval t;
  t.tv_sec=0;
  t.tv_usec=1000000;  //设置modbus超时时间为1000毫秒  
  modbus_set_response_timeout(mb,&t);

  int regs=modbus_read_registers(mb,100,5,tab_reg);


  printf("%d %d %d %d %d\n",regs,tab_reg[0],tab_reg[1],tab_reg[2],tab_reg[3],tab_reg[4]);

  modbus_close(mb);
  modbus_free(mb);
  return 0;
}


