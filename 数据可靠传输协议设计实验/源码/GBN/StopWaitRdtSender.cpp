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
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}

	if (this->expectSequenceNumberSend < base + N) {
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].acknum = -1; //忽略该字段
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].checksum = 0;
		memcpy(this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength]);
		
		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength]);
		if (base == this->expectSequenceNumberSend) {
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);			//启动发送基序列方定时器
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[this->expectSequenceNumberSend % Seqlength]);		//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		this->expectSequenceNumberSend++;
		if (this->expectSequenceNumberSend == base + N) {
			this->waitingState = true;//进入等待状态
		}
	}//进入等待状态
	return true;
}


void StopWaitRdtSender::receive(const Packet &ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base) {

		pUtils->printPacket("发送方正确收到确认", ackPkt);
		pns->stopTimer(SENDER, base);	//关闭定时器

		base = ackPkt.acknum + 1;  //窗口滑动
		this->waitingState = false;

		if (base != this->expectSequenceNumberSend){ //还没有接受完
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);
		}

		cout << "滑动窗口：" << '[' << ' ';
		for (int i = base; i < base + N; i++) {
			cout << i << ' ';
		}
		cout << ']' << endl;
	}
	else {
		if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("发送方收到的ACK损坏", ackPkt);
		}
		else {
			pUtils->printPacket("发送方没有收到正确的序号", ackPkt);
		}
	}
}


void StopWaitRdtSender::timeoutHandler(int seqNum) {
	pns->stopTimer(SENDER, seqNum);								//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	//唯一一个定时器,无需考虑seqNum
	for (int i = base; i < this->expectSequenceNumberSend; i++) {
		pUtils->printPacket("发送方定时器时间到，重发报文", this->packetWaitingAck[i % Seqlength]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i % Seqlength]);			//重新发送数据包
	}
}
