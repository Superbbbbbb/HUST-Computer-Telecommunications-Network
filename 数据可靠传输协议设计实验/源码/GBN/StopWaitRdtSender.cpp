#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"


StopWaitRdtSender::StopWaitRdtSender():expectSequenceNumberSend(0),waitingState(false),base(0)
{
	for (int i = 0; i < Seqlength; i++) {
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

	if (this->expectSequenceNumberSend < base + N) {
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].checksum = 0;
		memcpy(this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength]);
		
		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength]);
		if (base == this->expectSequenceNumberSend) {
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);			//�������ͻ����з���ʱ��
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength]);		//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		this->expectSequenceNumberSend++;
		if (this->expectSequenceNumberSend == base + N) {
			this->waitingState = true;//����ȴ�״̬
		}
	}//����ȴ�״̬
	return true;
}


void StopWaitRdtSender::receive(const Packet &ackPkt) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base) {

		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		pns->stopTimer(SENDER, base);	//�رն�ʱ��

		base = ackPkt.acknum + 1;  //���ڻ���
		this->waitingState = false;

		if (base != this->expectSequenceNumberSend){ //��û�н�����
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);
		}

		cout << "�������ڣ�" << '[' << ' ';
		for (int i = base; i < base + N; i++) {
			cout << i << ' ';
		}
		cout << ']' << endl;
	}
	else {
		if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("���ͷ��յ���ACK��", ackPkt);
		}
		else {
			pUtils->printPacket("���ͷ�û���յ���ȷ�����", ackPkt);
		}
	}
}


void StopWaitRdtSender::timeoutHandler(int seqNum) {
	pns->stopTimer(SENDER, seqNum);								//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	//Ψһһ����ʱ��,���迼��seqNum
	for (int i = base; i < this->expectSequenceNumberSend; i++) {
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط�����", this->packetWaitingAck[i % Seqlength]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i % Seqlength]);			//���·������ݰ�
	}
}
