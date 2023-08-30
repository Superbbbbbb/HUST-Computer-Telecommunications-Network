#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtReceiver.h"


StopWaitRdtReceiver::StopWaitRdtReceiver():expectSequenceNumberRcvd(N),base(0)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);

	for (int i = 0; i < Seqlength; i++) {
		packetFlag[i] = false;
	}
}


StopWaitRdtReceiver::~StopWaitRdtReceiver()
{
}

void StopWaitRdtReceiver::receive(const Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确
	if (checkSum == packet.checksum) {
		if (packet.seqnum >= base - N && packet.seqnum < expectSequenceNumberRcvd) {
			if (packet.seqnum == base) {
				pUtils->printPacket("接收方正确收到发送方的报文", packet);

				ReceivedPacket[packet.seqnum % Seqlength].acknum = 0;
				ReceivedPacket[packet.seqnum % Seqlength] = packet;
				packetFlag[packet.seqnum % Seqlength] = true;

				//取出Message，向上递交给应用层
				while (packetFlag[base % Seqlength]){
					Message msg;
					memcpy(msg.data, ReceivedPacket[base % Seqlength].payload, sizeof(ReceivedPacket[base % Seqlength].payload));
					pns->delivertoAppLayer(RECEIVER, msg);

					ReceivedPacket[packet.seqnum % Seqlength].acknum = -1;
					packetFlag[base % Seqlength] = false;//释放缓存区
					packetFlag[expectSequenceNumberRcvd % Seqlength] = false;

					base++;
					expectSequenceNumberRcvd++;
				}
			}
			else if (packet.seqnum > base && packet.seqnum < expectSequenceNumberRcvd) {    //不是base
				pUtils->printPacket("接收方已缓存发送方的报文", packet);
				ReceivedPacket[packet.seqnum % Seqlength] = packet;    //放到缓存区
				packetFlag[packet.seqnum % Seqlength] = true;
			}
			else if (packet.seqnum < base){
				pUtils->printPacket("接收方正确收到已确认的过时报文", packet);
			}

			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
		}
	}
	else {
		pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
	}
}