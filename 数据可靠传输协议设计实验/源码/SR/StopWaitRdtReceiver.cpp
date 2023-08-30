#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtReceiver.h"


StopWaitRdtReceiver::StopWaitRdtReceiver():expectSequenceNumberRcvd(N),base(0)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
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
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ
	if (checkSum == packet.checksum) {
		if (packet.seqnum >= base - N && packet.seqnum < expectSequenceNumberRcvd) {
			if (packet.seqnum == base) {
				pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);

				ReceivedPacket[packet.seqnum % Seqlength].acknum = 0;
				ReceivedPacket[packet.seqnum % Seqlength] = packet;
				packetFlag[packet.seqnum % Seqlength] = true;

				//ȡ��Message�����ϵݽ���Ӧ�ò�
				while (packetFlag[base % Seqlength]){
					Message msg;
					memcpy(msg.data, ReceivedPacket[base % Seqlength].payload, sizeof(ReceivedPacket[base % Seqlength].payload));
					pns->delivertoAppLayer(RECEIVER, msg);

					ReceivedPacket[packet.seqnum % Seqlength].acknum = -1;
					packetFlag[base % Seqlength] = false;//�ͷŻ�����
					packetFlag[expectSequenceNumberRcvd % Seqlength] = false;

					base++;
					expectSequenceNumberRcvd++;
				}
			}
			else if (packet.seqnum > base && packet.seqnum < expectSequenceNumberRcvd) {    //����base
				pUtils->printPacket("���շ��ѻ��淢�ͷ��ı���", packet);
				ReceivedPacket[packet.seqnum % Seqlength] = packet;    //�ŵ�������
				packetFlag[packet.seqnum % Seqlength] = true;
			}
			else if (packet.seqnum < base){
				pUtils->printPacket("���շ���ȷ�յ���ȷ�ϵĹ�ʱ����", packet);
			}

			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		}
		else {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų���", packet);
		}
	}
	else {
		pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
	}
}