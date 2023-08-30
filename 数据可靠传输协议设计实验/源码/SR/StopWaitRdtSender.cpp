#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"


StopWaitRdtSender::StopWaitRdtSender():expectSequenceNumberSend(0),waitingState(false),base(0)
{
	for (int i = 0; i < Seqlength; i++) {
		ackFlag[i] = false;
		this->packetWaitingAck[i].seqnum = -1;
	}
}


StopWaitRdtSender::~StopWaitRdtSender()
{
}


bool StopWaitRdtSender::getWaitingState() {
	return waitingState;
}


bool StopWaitRdtSender::send(const Message &message) {
	if (this->waitingState) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	if (expectSequenceNumberSend < base + N) {
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Seqlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Seqlength]);
		ackFlag[expectSequenceNumberSend % Seqlength] = false;

		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[expectSequenceNumberSend % Seqlength]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Seqlength]);		//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);			//�������ͻ����з���ʱ��
		expectSequenceNumberSend++;
		if (expectSequenceNumberSend == base + N) {
			this->waitingState = true;//����ȴ�״̬
		}
	}
	return true;
}

void StopWaitRdtSender::receive(const Packet &ackPkt) {
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
		if (checkSum == ackPkt.checksum) {

			if (ackPkt.acknum == base) {
				pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
				pns->stopTimer(SENDER, ackPkt.acknum);
				this->ackFlag[base % Seqlength] = true;
				while (ackFlag[base % Seqlength]) {
					ackFlag[base % Seqlength] = false;
					base++;
				}
				this->waitingState = false;

				cout << "�������ڣ�" << '[' << ' ';
				for (int i = base; i < base + N; i++) {
					cout << i << ' ';
				}
				cout << ']' << endl;

			}
			else if (ackPkt.acknum > base && !ackFlag[ackPkt.acknum % Seqlength]) {
				pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
				pns->stopTimer(SENDER, ackPkt.acknum);
				this->ackFlag[ackPkt.acknum % Seqlength] = true;
			}
			else{
				pUtils->printPacket("���ͷ��յ��ظ�ACK", ackPkt);
			}
		}
		else {
			pUtils->printPacket("���ͷ��յ���ACK��", ackPkt);
		}
}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
	pns->stopTimer(SENDER, seqNum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط�����", this->packetWaitingAck[seqNum % Seqlength]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum % Seqlength]);			//���·������ݰ�
}
