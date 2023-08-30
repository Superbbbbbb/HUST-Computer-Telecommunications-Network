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
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}

	if (expectSequenceNumberSend < base + N) {
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].acknum = -1; //忽略该字段
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].seqnum = this->expectSequenceNumberSend;
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Seqlength].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Seqlength].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Seqlength]);
		ackFlag[expectSequenceNumberSend % Seqlength] = false;

		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend % Seqlength]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Seqlength]);		//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		pns->startTimer(SENDER, Configuration::TIME_OUT, expectSequenceNumberSend);			//启动发送基序列方定时器
		expectSequenceNumberSend++;
		if (expectSequenceNumberSend == base + N) {
			this->waitingState = true;//进入等待状态
		}
	}
	return true;
}

void StopWaitRdtSender::receive(const Packet &ackPkt) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
		if (checkSum == ackPkt.checksum) {

			if (ackPkt.acknum == base) {
				pUtils->printPacket("发送方正确收到确认", ackPkt);
				pns->stopTimer(SENDER, ackPkt.acknum);
				this->ackFlag[base % Seqlength] = true;
				while (ackFlag[base % Seqlength]) {
					ackFlag[base % Seqlength] = false;
					base++;
				}
				this->waitingState = false;

				cout << "滑动窗口：" << '[' << ' ';
				for (int i = base; i < base + N; i++) {
					cout << i << ' ';
				}
				cout << ']' << endl;

			}
			else if (ackPkt.acknum > base && !ackFlag[ackPkt.acknum % Seqlength]) {
				pUtils->printPacket("发送方正确收到确认", ackPkt);
				pns->stopTimer(SENDER, ackPkt.acknum);
				this->ackFlag[ackPkt.acknum % Seqlength] = true;
			}
			else{
				pUtils->printPacket("发送方收到重复ACK", ackPkt);
			}
		}
		else {
			pUtils->printPacket("发送方收到的ACK损坏", ackPkt);
		}
}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
	pns->stopTimer(SENDER, seqNum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
	pUtils->printPacket("发送方定时器时间到，重发报文", this->packetWaitingAck[seqNum % Seqlength]);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[seqNum % Seqlength]);			//重新发送数据包
}
