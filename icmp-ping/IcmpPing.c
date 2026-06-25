#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/Ip4.h>                    // EFI_IP4_PROTOCOL
#include <Protocol/ServiceBinding.h>         // EFI_SERVICE_BINDING_PROTOCOL
#include <Protocol/Ip4Config2.h>             // EFI_IP4_CONFIG2_PROTOCOL


typedef struct {
  UINT8 Type;
  UINT8 Code;
  UINT16 Checksum;
  UINT16 Identifier;
  UINT16 SequenceNumber;
  UINT8 Data[32];

} ICMP_ECHO_REQUEST;

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  
  EFI_SERVICE_BINDING_PROTOCOL  *Ip4Sb;
  EFI_STATUS                    Status;
  EFI_IP4_PROTOCOL              *Ip4;
  EFI_HANDLE                    Ip4Handle;
  EFI_IP4_CONFIG_DATA           ConfigIp4;
  EFI_IP4_COMPLETION_TOKEN      Token;
  EFI_IP4_TRANSMIT_DATA         TxData;
  EFI_IP4_FRAGMENT_DATA         Fragment;
  ICMP_ECHO_REQUEST             IcmpPkt;
  EFI_IPv4_ADDRESS              Gateway;
  EFI_IPv4_ADDRESS              Zero;
 
  Status =  gBS->LocateProtocol(&gEfiIp4ServiceBindingProtocolGuid, NULL, (VOID**)&Ip4Sb);

  if (Status != EFI_SUCCESS){
    Print(L"Ip4ServiceBinding not found: 0x%lx\n", Status);
    return Status;

  

  }
  Print(L"Ip4ServiceBinding found!\n");
  Print(L"Ip4Sb address: 0x%lx\n", (UINT64)Ip4Sb);
  
  
  Ip4Handle = NULL;
  Ip4Sb->CreateChild(
    Ip4Sb,
    &Ip4Handle
);

  Status = gBS->HandleProtocol(Ip4Handle,&gEfiIp4ProtocolGuid,(VOID**)&Ip4);
  
  ConfigIp4.DefaultProtocol = 1;
  ConfigIp4.UseDefaultAddress = FALSE;
  ConfigIp4.TimeToLive = 64;
  ConfigIp4.AcceptIcmpErrors = TRUE;
  ConfigIp4.AcceptAnyProtocol = FALSE;

  ConfigIp4.StationAddress.Addr[0] = 10;
  ConfigIp4.StationAddress.Addr[1] = 0;
  ConfigIp4.StationAddress.Addr[2] = 2;
  ConfigIp4.StationAddress.Addr[3] = 15;


  ConfigIp4.SubnetMask.Addr[0] = 255;
  ConfigIp4.SubnetMask.Addr[1] = 255;
  ConfigIp4.SubnetMask.Addr[2] = 255;
  ConfigIp4.SubnetMask.Addr[3] = 0;

  Status = Ip4->Configure(Ip4, &ConfigIp4);
if (Status != EFI_SUCCESS) {
    Print(L"Configure failed: 0x%lx\n", Status);
    return Status;
}


  Print(L"#################\n");
  Print(L"Protocol Configs\n");
  Print(L"#################\n");

  Print(L"Station Address : %d.%d.%d.%d\n",
    ConfigIp4.StationAddress.Addr[0],
    ConfigIp4.StationAddress.Addr[1],
    ConfigIp4.StationAddress.Addr[2],
    ConfigIp4.StationAddress.Addr[3]
);

  Print(L"Protocol : %d\n",
    ConfigIp4.DefaultProtocol
  );



  
  // ICMP Packet config
  IcmpPkt.Type = 8;
  IcmpPkt.Code = 0;
  IcmpPkt.SequenceNumber = 1;
  IcmpPkt.Identifier = 0111;
  IcmpPkt.Checksum = 0;
  IcmpPkt.Data[0] = 'V';
  IcmpPkt.Data[1] = 'E';
  IcmpPkt.Data[2] = 'S';

  // Fragment Configs
  Fragment.FragmentLength = sizeof(ICMP_ECHO_REQUEST);
  Fragment.FragmentBuffer = &IcmpPkt;

 

  // TxData Dest.
  TxData.DestinationAddress.Addr[0] = 8;
  TxData.DestinationAddress.Addr[1] = 8;
  TxData.DestinationAddress.Addr[2] = 8;
  TxData.DestinationAddress.Addr[3] = 8;

  // TxData Other configs.
  TxData.FragmentCount = 1;
  TxData.FragmentTable[0] = Fragment;
  TxData.TotalDataLength = sizeof(ICMP_ECHO_REQUEST);
  TxData.OverrideData = NULL;
  // I was getting error before adding this (transmit)= 0x2
  TxData.OptionsLength = 0;
  TxData.OptionsBuffer = NULL;

  // Token Configs
  Token.Packet.TxData = &TxData;

  Status = gBS->CreateEvent(
    0,
    0,
    NULL,
    NULL,
    &Token.Event
);
  if (Status != EFI_SUCCESS){
    Print(L"Token Status :", Status);
    return Status;
  }

  Gateway.Addr[0] = 10;
  Gateway.Addr[1] = 0;
  Gateway.Addr[2] = 2;
  Gateway.Addr[3] = 2;   // QEMU gateway

  Zero.Addr[0] = 0;
  Zero.Addr[1] = 0;
  Zero.Addr[2] = 0;
  Zero.Addr[3] = 0;

  Ip4->Routes(Ip4, FALSE, &Zero, &Zero, &Gateway);


  Status = Ip4->Transmit(Ip4,&Token);
  if (Status != EFI_SUCCESS){
    Print(L"Transmit Failed 0x%1x\n", Status);
    return Status;
  }

  while (Token.Status == EFI_NOT_READY){
    Ip4->Poll(Ip4);
  }

  Print(L"\r\nPing sent to Destination Adress\r\n");
  return EFI_SUCCESS;
}
