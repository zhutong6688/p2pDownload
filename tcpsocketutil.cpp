#include "tcpsocketutil.h"

TCPSocketUtil::TCPSocketUtil()
{

}

TCPSocketUtil::TCPSocketUtil(quint16 hostPort, quint16 fileHostPort, bool openHost, bool openGuest, QString fileDir, QString fileType, qint32 groupSize) : hostPort(hostPort), fileHostPort(fileHostPort), openHost(openHost), openGuest(openGuest), fileDir(fileDir), fileType(fileType), sliceSize(groupSize)
{
    this->connectedNum = 0;
    this->fileConnectedNum = 0;
    this->friendName = 0;
}

TCPSocketUtil::~TCPSocketUtil()
{
    this->partnerFileIndex.clear();

    this->guestPort.clear();
    this->fileGuestPort.clear();

    for (QMap<qint32, P2PTcpSocket *>::iterator guestsIt = this->guests.begin(); guestsIt != this->guests.end(); guestsIt++) {
        delete guestsIt.value();
        guestsIt.value() = nullptr;
    }
    this->guests.clear();

    for (QMap<qint32, P2PTcpSocket *>::iterator fileGuestsIt = this->fileGuests.begin(); fileGuestsIt != this->fileGuests.end(); fileGuestsIt++) {
        delete fileGuestsIt.value();
        fileGuestsIt.value() = nullptr;
    }
    this->fileGuests.clear();

    closeHost();
    closeFileHost();

    for (QMap<qint32, Client *>::iterator parntersMapIt = this->parntersMap.begin(); parntersMapIt != this->parntersMap.end(); parntersMapIt++) {
        delete parntersMapIt.value();
        parntersMapIt.value() = nullptr;
    }
    this->parntersMap.clear();
}

bool TCPSocketUtil::bindClients(QVector<Client *> clients, QVector<quint16> ports, QVector<quint16> filePorts)
{
    if (clients.empty() || ports.empty() || filePorts.empty()) {
        qDebug() << "TCPSocketUtil::bindClients " << "初始化伙伴客户端信息失败" << endl;
        return false;
    }

    int len = clients.size();
    for (int i = 0; i < len; i++) {
        addClient(clients.at(i), ports.at(i), filePorts.at(i));
    }

    return true;
}

bool TCPSocketUtil::addClient(Client * client, quint16 port, quint16 filePort)
{
    if (this->parntersMap.contains(client->getId())) {
        qDebug() << "TCPSocketUtil::addClient " << "伙伴客户端不允许重复加入队列" << client->getId() << endl;
        return false;
    }

    this->parntersMap[client->getId()] = client;
    this->guestPort[client->getId()] = port;
    this->fileGuestPort[client->getId()] = filePort;

    bool guestSucceedBind = stablishGuest(client->getId());
    bool fileGuestSucceedBind = stablishFileGuest(client->getId());

    if (!guestSucceedBind || !fileGuestSucceedBind) {
        qDebug() << "TCPSocketUtil::addClient " << "无法添加新的朋友客户端" << endl;
        return false;
    }

    return true;
}

bool TCPSocketUtil::stablishHost()
{
    if (!this->openHost) {
        qDebug() << "TCPSocketUtil::stablishHost " << "未开启TCP服务器权限" << endl;
        return false;
    }

    createHost();

    qDebug() << this->hostPort << this->fileHostPort << endl;
    if (!listenPort()) {
        qDebug() << "TCPSocketUtil::stablishHost " << "端口侦听失败" << endl;
        qDebug() << "TCPSocketUtil::stablishHost " << this->host->errorString() << endl;

        return false;
    }

    connect(host, SIGNAL(newConnection()), this, SLOT(recFromPartner()));
    return true;
}

bool TCPSocketUtil::stablishGuest(qint32 partnerId)
{
    if (!this->openGuest) {
        qDebug() << "TCPSocketUtil::stablishGuest " << "未开启TCP客户端权限" << endl;
        return false;
    }

    createGuest(partnerId);
    connectToFriend(partnerId);

    if (!connectToFriend(partnerId)) {
        qDebug() << "TCPSocketUtil::stablishGuest " << "连接朋友客户端失败 " << partnerId << endl;

        closeGuest(partnerId);
        return false;
    }

    return true;
}

bool TCPSocketUtil::stablishFileHost()
{
    if (!this->openHost) {
        qDebug() << "TCPSocketUtil::stablishFileHost " << "未开启TCP服务器权限" << endl;
        return false;
    }

    createFileHost();

    if (!listenFilePort()) {
        qDebug() << "TCPSocketUtil::stablishFileHost " << "端口侦听失败" << endl;
        qDebug() << "TCPSocketUtil::stablishFileHost " << this->host->errorString() << endl;

        return false;
    }

    connect(fileHost, SIGNAL(newConnection()), this, SLOT(recFileFromPartner()));
    return true;
}

bool TCPSocketUtil::stablishFileGuest(qint32 partnerId)
{
    if (!this->openGuest) {
        qDebug() << "TCPSocketUtil::stablishFileGuest " << "未开启TCP客户端权限" << endl;
        return false;
    }

    createFileGuest(partnerId);

    if (!connectToFileFriend(partnerId)) {
        qDebug() << "TCPSocketUtil::stablishFileGuest " << "连接朋友客户端失败 " << partnerId << endl;

        closeFileGuest(partnerId);
        return false;
    }

    return true;
}

bool TCPSocketUtil::createHost()
{
    this->host = new QTcpServer();
    return true;
}

bool TCPSocketUtil::createGuest(qint32 partnerId)
{
    if (this->guests.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::createGuest " << "指定连接该朋友客户端的P2PTcpSocket对象此前已建立 " << partnerId << endl;
        return false;
    }

    this->guests[partnerId] = new P2PTcpSocket();
    return true;
}

bool TCPSocketUtil::closeHost()
{
    for (QMap<qint32, P2PTcpSocket *>::iterator it = this->partnerConnections.begin(); it != this->partnerConnections.end(); it++) {
        it.value()->close();
        delete it.value();
        it.value() = nullptr;
    }

    this->partnerConnections.clear();

    this->host->close();
    delete this->host;

    return true;
}

bool TCPSocketUtil::closeGuest(qint32 partnerId)
{
    if (!disConnectToFriend(partnerId)) {
        qDebug() << "TCPSocketUtil::closeGuest " << "错误尝试关闭和朋友客户端之间的TCP连接 " << partnerId << endl;
    }

    delete this->guests[partnerId];
    this->guests[partnerId] = nullptr;

    return true;
}

bool TCPSocketUtil::createFileHost()
{
    this->fileHost = new QTcpServer();
    return true;
}

bool TCPSocketUtil::createFileGuest(qint32 partnerId)
{
    if (this->fileGuests.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::createFileGuest " << "指定连接该朋友客户端的P2PTcpSocket对象此前已建立 " << partnerId << endl;
        return false;
    }

    this->fileGuests[partnerId] = new P2PTcpSocket();
    return true;
}

bool TCPSocketUtil::closeFileHost()
{
    for (QMap<qint32, P2PTcpSocket *>::iterator it = this->partnerFileConnections.begin(); it != this->partnerFileConnections.end(); it++) {
        it.value()->close();
        delete it.value();
        it.value() = nullptr;
    }

    this->partnerFileConnections.clear();

    this->fileHost->close();
    delete this->fileHost;

    return true;
}

bool TCPSocketUtil::closeFileGuest(qint32 partnerId)
{
    if (!disConnectToFileFriend(partnerId)) {
        qDebug() << "TCPSocketUtil::closeFileGuest " << "错误尝试关闭和朋友客户端之间的TCP连接 " << partnerId << endl;
    }

    delete this->fileGuests[partnerId];
    this->fileGuests[partnerId] = nullptr;

    return true;
}

bool TCPSocketUtil::listenPort()
{
    if (!this->host->listen(QHostAddress::Any, hostPort)) {
        qDebug() << "TCPSocketUtil::listenPort " << "端口侦听失败" << endl;
        qDebug() << "TCPSocketUtil::listenPort " << this->host->errorString() << endl;

        this->host->close();
        return false;
    }

    return true;
}

bool TCPSocketUtil::connectToFriend(qint32 partnerId)
{
    if (!this->guests.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::connectToFriend " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << partnerId << endl;
        createGuest(partnerId);
    }

    if (!this->parntersMap.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::connectToFriend " << "无法获取指定伙伴的ip地址 " << partnerId << endl;
        return false;
    }

    if (!this->guestPort.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::connectToFriend " << "未指定侦听的端口 " << partnerId << endl;
        return false;
    }

    if (!this->guests[partnerId]->bind(QHostAddress::LocalHost, this->guestPort[partnerId])) {
        qDebug() << "TCPSocketUtil::connectToFriend " << "端口侦听失败 " << partnerId << endl;
        qDebug() << "TCPSocketUtil::connectToFriend " << this->guests[partnerId]->errorString() << endl;

        this->guests[partnerId]->close();
        return false;
    }

    this->guests[partnerId]->connectToHost(this->parntersMap[partnerId]->getIP(), this->parntersMap[partnerId]->getPort());
    this->connectedNum++;

    connect(this->guests[partnerId], SIGNAL(readyRead()), this->guests[partnerId], SLOT(ensureReadyRead()));
    connect(this->guests[partnerId], SIGNAL(error(QAbstractSocket::SocketError)), this->guests[partnerId], SLOT(ensureError(QAbstractSocket::SocketError)));
    connect(this->guests[partnerId], SIGNAL(disconnected()), this->guests[partnerId], SLOT(ensureDisconnected));

    connect(this->guests[partnerId], SIGNAL(readyReadFromOthers(qint32)), this, SLOT(recFromFriend(qint32)));
    connect(this->guests[partnerId], SIGNAL(socketErrorOfOthers(QAbstractSocket::SocketError, qint32)), this, SLOT(failToHelpFriend(QAbstractSocket::SocketError, qint32)));
    connect(this->guests[partnerId], SIGNAL(disconnectedFromOthers(qint32)), this, SLOT(disConnectToFriend(qint32)));

    return true;
}

bool TCPSocketUtil::disConnectToFriend(qint32 friendId)
{
    if (!this->guests.contains(friendId)) {
        qDebug() << "TCPSocketUtil::disConnectToFriend " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << friendId << endl;
        return false;
    }

    this->guests[friendId]->disconnectFromHost();
    return true;
}

bool TCPSocketUtil::listenFilePort()
{
    if (!this->fileHost->listen(QHostAddress::LocalHost, fileHostPort)) {
        qDebug() << "TCPSocketUtil::listenFilePort " << "端口侦听失败" << endl;
        qDebug() << "TCPSocketUtil::listenFilePort " << this->fileHost->errorString() << endl;
        this->fileHost->close();

        return false;
    }

    return true;
}


bool TCPSocketUtil::connectToFileFriend(qint32 partnerId)
{
    if (!this->fileGuests.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::connectFileToPartner " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << partnerId << endl;
        createFileGuest(partnerId);
    }

    if (!this->parntersMap.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::connectFileToPartner " << "无法获取指定伙伴的ip地址" << partnerId << endl;
        return false;
    }

    if (!this->fileGuestPort.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::connectToPartner " << "未指定侦听的端口 " << partnerId << endl;
        return false;
    }

    if (!this->fileGuests[partnerId]->bind(QHostAddress::LocalHost, this->fileGuestPort[partnerId])) {
        qDebug() << "TCPSocketUtil::connectToPartner " << "端口侦听失败 " << partnerId << endl;
        qDebug() << "TCPSocketUtil::connectToPartner " << this->fileGuests[partnerId]->errorString() << endl;

        this->fileGuests[partnerId]->close();
        return false;
    }

    this->fileGuests[partnerId]->connectToHost(this->parntersMap[partnerId]->getIP(), this->parntersMap[partnerId]->getFilePort());
    this->fileConnectedNum++;

    connect(this->fileGuests[partnerId], SIGNAL(readyRead()), this->fileGuests[partnerId], SLOT(ensureReadyRead()));
    connect(this->fileGuests[partnerId], SIGNAL(error(QAbstractSocket::SocketError)), this->fileGuests[partnerId], SLOT(ensureError(QAbstractSocket::SocketError)));
    connect(this->fileGuests[partnerId], SIGNAL(disconnected()), this->fileGuests[partnerId], SLOT(ensureDisconnected()));

    connect(this->fileGuests[partnerId], SIGNAL(readyReadFromOthers(qint32)), this, SLOT(recFromFileFriend(qint32)));
    connect(this->fileGuests[partnerId], SIGNAL(socketErrorOfOthers(QAbstractSocket::SocketError, qint32)), this, SLOT(failToHelpFriend(QAbstractSocket::SocketError, qint32)));
    connect(this->fileGuests[partnerId], SIGNAL(disconnectedFromOthers(qint32)), this, SLOT(disConnectToFileFriend(qint32)));

    return true;
}

bool TCPSocketUtil::disConnectToFileFriend(qint32 friendId)
{
    if (!this->fileGuests.contains(friendId)) {
        qDebug() << "TCPSocketUtil::disConnectToFileFriend " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << friendId << endl;
        return false;
    }

    this->fileGuests[friendId]->disconnectFromHost();
    return true;
}

bool TCPSocketUtil::newConnectionWithPartner()
{
    P2PTcpSocket * vistor = dynamic_cast<P2PTcpSocket *>(this->host->nextPendingConnection());
    qint32 vistorId = vistor->peerName().toInt();

    if (!this->partnerConnections.contains(vistorId) && !this->parntersMap.contains(vistorId)) {
        qDebug() << "TCPSocketUtil::newConnectionWithPartner " << "陌生客户端非法访问" << endl;
        return false;
    }

    this->partnerConnections[vistorId] = vistor;
    this->partnerConnections[vistorId]->setId(vistorId);

    connect(this->partnerConnections[vistorId], SIGNAL(readyRead()), this->partnerConnections[vistorId], SLOT(ensureReadyRead()));
    connect(this->partnerConnections[vistorId], SIGNAL(error(QAbstractSocket::SocketError)), this->partnerConnections[vistorId], SLOT(ensureError(QAbstractSocket::SocketError)));
    connect(this->partnerConnections[vistorId], SIGNAL(disconnected()), this->partnerConnections[vistorId], SLOT(ensureDisconnected));

    connect(this->partnerConnections[vistorId], SIGNAL(readyReadFromOthers(qint32)), this, SLOT(recFromPartner(qint32)));
    connect(this->partnerConnections[vistorId], SIGNAL(socketErrorOfOthers(QAbstractSocket::SocketError, qint32)), this, SLOT(failToGetHelpFromPartner(QAbstractSocket::SocketError, qint32)));
    connect(this->partnerConnections[vistorId], SIGNAL(disconnectedFromOthers(qint32)), this, SLOT(disConnectToPartner(qint32)));

    return true;
}

bool TCPSocketUtil::disConnectToPartner(qint32 partnerId)
{
    if (!this->partnerConnections.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::disConnectToPartner " << "指定连接该伙伴客户端的P2PTcpSocket对象尚未建立 " << partnerId << endl;
        return false;
    }

    this->partnerConnections[partnerId]->disconnectFromHost();
    return true;
}

bool TCPSocketUtil::recFromPartner(qint32 partnerId)
{    
    if (!this->partnerConnections.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::recFromPartner " << "P2PTcpSocket::readyReadFromPartner信号的partnerId错误 " << partnerId << endl;
        return false;
    }

    QByteArray msg = this->partnerConnections[partnerId]->readAll();
    QJsonObject jsonMsg = QJsonDocument::fromJson(msg).object();

    if (jsonMsg.value(MSGTYPE).isUndefined()) {
        qDebug() << "TCPSocketUtil::recFromPartner " << "无法解析伙伴客户端发来的消息类型" << endl;
        return false;
    }

    if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::P2PPUNCH) {
        // P2P打洞完成后，告知伙伴客户端下载地址
        emit timeToInitialTaskForPartner(partnerId);

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::ISALIVE) {
        // 伙伴客户端确认存活
        emit whetherToStopTask(partnerId);

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::AGREETOHELP) {
        // 伙伴客户端接受下载协助请求，准备为伙伴客户端分配下载任务
        emit timeForFirstTaskForPartner(partnerId);

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::REFUSETOHELP) {
        // 伙伴客户端拒绝下载协助请求，动态调整任务分配计划
        emit refuseToOfferHelpForPartner(partnerId);

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::TASKFINISH) {
        if (jsonMsg.value(TOKEN).isUndefined()) {
            qDebug() << "TCPSocketUtil::recFromPartner " << "伙伴客户端发来的消息不完整" << endl;
            return false;
        }

        qint32 token = qint32(jsonMsg.value(TOKEN).toInt());
        // 文件传送完成，准备在主机组装
        emit timeForNextTaskForPartner(partnerId, token);

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::TASKFAILURE) {
        if (jsonMsg.value(TOKEN).isUndefined()) {
            qDebug() << "TCPSocketUtil::recFromPartner " << "伙伴客户端发来的消息不完整" << endl;
            return false;
        }

        qint32 token = qint32(jsonMsg.value(TOKEN).toInt());
        // 文件传送失败，准备重新分配任务或重传
        emit taskFailureForPartner(partnerId, token);
    }

    return true;
}

bool TCPSocketUtil::sendToPartner(qint32 partnerId, CommMsg & msg)
{
    if (!this->partnerConnections.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::sendToPartner " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << partnerId << endl;
        return false;
    }

    if (msg.getMsgType() != TCPCtrlMsgType::AREYOUALIVE && msg.getMsgType() != TCPCtrlMsgType::ASKFORHELP && msg.getMsgType() != TCPCtrlMsgType::DOWNLOADTASK && msg.getMsgType() == TCPCtrlMsgType::THANKYOURHELP && msg.getMsgType() == TCPCtrlMsgType::ENDYOURHELP) {
        qDebug() << "TCPSocketUtil::sendToPartner " << "向伙伴客户端发送的消息类型不合法 " << partnerId << endl;
        return false;
    }

    if (msg.getMsgType() == TCPCtrlMsgType::DOWNLOADTASK) {
        this->partnerFileIndex[msg.getToken()] = 0;
    }

    // 请求伙伴客户端协助下载, 分配下载任务, 通知伙伴客户端可以继续传送文件, 下载已完成，终止任务
    this->partnerConnections[partnerId]->write(msg.toMsg());
    return true;
}

bool TCPSocketUtil::failToGetHelpFromPartner(QAbstractSocket::SocketError error, qint32 partnerId)
{
    qDebug() << "TCPSocketUtil::failToGetHelpFromPartner " << "无法和伙伴客户端建立稳定连接 " << partnerId << endl;
    qDebug() << "TCPSocketUtil::failToGetHelpFromPartner " << this->partnerConnections[partnerId]->errorString() << endl;
    qDebug() << "TCPSocketUtil::failToGetHelpFromPartner " << "SocketError " << error << endl;

    this->partnerConnections[partnerId]->close();
    delete this->partnerConnections[partnerId];
    this->partnerConnections[partnerId] = nullptr;
    this->partnerConnections.remove(partnerId);

    return true;
}

bool TCPSocketUtil::recFromFriend(qint32 friendId)
{
    QJsonObject jsonMsg = QJsonDocument::fromJson(this->guests[friendId]->readAll()).object();
    if (jsonMsg.value(MSGTYPE).isUndefined()) {
        qDebug() << "TCPSocketUtil::recFromFriend " << "无法解析朋友客户端发来的消息类型" << endl;
        return false;
    }

    if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::AREYOUALIVE) {
        // 确认存活，评估下载进度
        emit tellTaskProcess(friendId);

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::ASKFORHELP) {
        if (jsonMsg.value(DOWNLOADADDRESS).isUndefined()) {
            qDebug() << "TCPSocketUtil::recFromFriend " << "朋友客户端发来的消息不完整" << endl;
            return false;
        }

        // 获取文件下载的目标地址和总大小
        emit whetherToHelpFriend(friendId, jsonMsg.value(DOWNLOADADDRESS).toString(), qint32(jsonMsg.value(LENMAX).toInt()));

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::DOWNLOADTASK) {
        if (jsonMsg.value(TOKEN).isUndefined() || jsonMsg.value(POS).isUndefined() || jsonMsg.value(LEN).isUndefined()) {
            qDebug() << "TCPSocketUtil::recFromFriend " << "朋友客户端发来的消息不完整" << endl;
            return false;
        }

        // 确定具体任务
        emit startToDownload(friendId, qint32(jsonMsg.value(TOKEN).toInt()), qint64(jsonMsg.value(POS).toInt()), qint32(jsonMsg.value(LEN).toInt()));

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::THANKYOURHELP) {
        if (jsonMsg.value(TOKEN).isUndefined() || jsonMsg.value(INDEX).isUndefined()) {
            qDebug() << "TCPSocketUtil::recFromFriend " << "朋友客户端发来的消息不完整" << endl;
            return false;
        }

       // 本次小块传送结束，伙伴客户端准备传送下一块
       emit timeForNextSliceForFriend(friendId, qint32(jsonMsg.value(TOKEN).toInt()), qint32(jsonMsg.value(INDEX).toInt()));

    } else if (static_cast<TCPCtrlMsgType>(jsonMsg.value(MSGTYPE).toInt()) == TCPCtrlMsgType::ENDYOURHELP) {
        if (jsonMsg.value(TOKEN).isUndefined()) {
            qDebug() << "TCPSocketUtil::recFromFriend " << "朋友客户端发来的消息不完整" << endl;
            return false;
        }

        // 结束任务，伙伴客户端停止传送文件
        emit taskHasFinishedForFriend(friendId, qint32(jsonMsg.value(TOKEN).toInt()));
    }

    return true;
}

bool TCPSocketUtil::sendToFriend(qint32 friendId, CommMsg & msg)
{
    if (!this->guests.contains(friendId)) {
        qDebug() << "TCPSocketUtil::sendToFriend " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << friendId << endl;
        createGuest(friendId);
    }

    if (msg.getMsgType() != TCPCtrlMsgType::P2PPUNCH && msg.getMsgType() != TCPCtrlMsgType::ISALIVE && msg.getMsgType() != TCPCtrlMsgType::AGREETOHELP && msg.getMsgType() == TCPCtrlMsgType::REFUSETOHELP && msg.getMsgType() == TCPCtrlMsgType::TASKFINISH && msg.getMsgType() == TCPCtrlMsgType::TASKFAILURE) {
        qDebug() << "TCPSocketUtil::sendToFriend " << "向朋友客户端发送的消息类型不合法 " << friendId << endl;
        return false;
    }

    this->guests[friendId]->write(msg.toMsg());
    return true;
}

bool TCPSocketUtil::failToHelpFriend(QAbstractSocket::SocketError error, qint32 friendId)
{
    qDebug() << "TCPSocketUtil::failToHelpFriend " << "无法和伙伴客户端建立稳定连接 " << friendId << endl;
    qDebug() << "TCPSocketUtil::failToHelpFriend " << this->guests[friendId]->errorString() << endl;
    qDebug() << "TCPSocketUtil::failToHelpFriend " << "SocketError " << error << endl;

    this->guests[friendId]->close();
    delete this->guests[friendId];
    this->guests[friendId] = nullptr;
    this->guests.remove(friendId);

    return true;
}

bool TCPSocketUtil::newConnectionWithFilePartner()
{
    P2PTcpSocket * vistor = dynamic_cast<P2PTcpSocket *>(this->fileHost->nextPendingConnection());
    qint32 vistorId = vistor->peerName().toInt();

    if (!this->partnerFileConnections.contains(vistorId) && !this->parntersMap.contains(vistorId)) {
        qDebug() << "TCPSocketUtil::newConnectionWithFilePartner " << "陌生客户端非法访问" << endl;
        return false;
    }

    this->partnerFileConnections[vistorId] = vistor;
    this->partnerFileConnections[vistorId]->setId(vistorId);

    connect(this->partnerFileConnections[vistorId], SIGNAL(readyRead()), this->partnerFileConnections[vistorId], SLOT(ensureReadyRead()));
    connect(this->partnerFileConnections[vistorId], SIGNAL(error(QAbstractSocket::SocketError)), this->partnerFileConnections[vistorId], SLOT(ensureError(QAbstractSocket::SocketError)));
    connect(this->partnerFileConnections[vistorId], SIGNAL(disconnected()), this->partnerFileConnections[vistorId], SLOT(ensureDisconnected));

    connect(this->partnerFileConnections[vistorId], SIGNAL(readyReadFromOthers(qint32)), this, SLOT(recFromFilePartner(qint32)));
    connect(this->partnerFileConnections[vistorId], SIGNAL(socketErrorOfOthers(QAbstractSocket::SocketError, qint32)), this, SLOT(failToGetHelpFromFilePartner(QAbstractSocket::SocketError, qint32)));
    connect(this->partnerFileConnections[vistorId], SIGNAL(disconnectedFromOthers(qint32)), this, SLOT(disConnectToFilePartner(qint32)));

    return true;
}

bool TCPSocketUtil::disConnectToFilePartner(qint32 partnerId)
{
    if (!this->partnerFileConnections.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::disConnectToFilePartner " << "指定连接该伙伴客户端的P2PTcpSocket对象尚未建立 " << partnerId << endl;
        return false;
    }

    this->partnerFileConnections[partnerId]->disconnectFromHost();
    return true;
}

bool TCPSocketUtil::recFromFilePartner(qint32 partnerId)
{
    if (!this->partnerConnections.contains(partnerId)) {
        qDebug() << "TCPSocketUtil::recFromFilePartner " << "P2PTcpSocket::readyReadFromPartner信号的partnerId错误 " << partnerId << endl;
        return false;
    }

    QByteArray msg = this->partnerFileConnections[partnerId]->readAll();
    TCPCtrlMsgType msgType = TCPCtrlMsgType(msg.left(1).toInt());

    if (msgType == TCPCtrlMsgType::TASKEXECUING) {
        qint32 token = qint32(msg.mid(1,4).toInt());
        qint32 index = qint32(msg.mid(5,4).toInt());
        qint8 lastOne = qint8(msg.mid(9,1).toInt());

        if (index != this->partnerFileIndex[token]) {
            qDebug() << "TCPSocketUtil::recFromFilePartner " << "伙伴客户端数据发送顺序错误 " << partnerId << " 失败的任务令牌 " << token << " 索引 " << index << endl;
            emit timeForNextSliceForPartner(partnerId, token, this->partnerFileIndex[token]);
            return false;
        }

        QFile * file = new QFile(fileDir + QString::number(token) + fileType);
        if (!file->open(QIODevice::WriteOnly)) {
            qDebug() << "TCPSocketUtil::recFromFilePartner " << "伙伴客户端数据发送失败 " << partnerId << " 失败的任务令牌 " << token << " 索引 " << index << endl;
            return false;
        }

        if (lastOne != 1 && (msg.length() - 10)< this->sliceSize) {
            qDebug() << "TCPSocketUtil::recFromFilePartner " << "伙伴客户端数据发送不完整 " << partnerId << " 失败的任务令牌 " << token << " 索引 " << index << endl;
            return false;
        }

        file->write(msg.mid(10));

        if (lastOne == 1) {
            this->partnerFileIndex.remove(token);
            emit taskHasFinishedForFriend(partnerId, token);
        }

        return true;
    }

    qDebug() << "TCPSocketUtil::recFromFilePartner " << "向伙伴客户端发送的消息类型不合法 " << partnerId << endl;
    return false;
}

bool TCPSocketUtil::failToGetHelpFromFilePartner(QAbstractSocket::SocketError error, qint32 partnerId)
{
    qDebug() << "TCPSocketUtil::failToGetHelpFromFilePartner " << "无法和伙伴客户端建立稳定连接 " << partnerId << endl;
    qDebug() << "TCPSocketUtil::failToGetHelpFromFilePartner " << this->partnerFileConnections[partnerId]->errorString() << endl;
    qDebug() << "TCPSocketUtil::failToGetHelpFromFilePartner " << "SocketError " << error << endl;

    this->partnerFileConnections[partnerId]->close();
    delete this->partnerFileConnections[partnerId];
    this->partnerFileConnections[partnerId] = nullptr;
    this->partnerFileConnections.remove(partnerId);

    return true;
}

bool TCPSocketUtil::sendToFileFriend(qint32 friendId, FileMsg & msg)
{
    if (!this->fileGuests.contains(friendId)) {
        qDebug() << "TCPSocketUtil::sendToFileFriend " << "指定连接该朋友客户端的P2PTcpSocket对象尚未建立 " << friendId << endl;
        createGuest(friendId);
    }

    if (!this->parntersMap.contains(friendId)) {
        qDebug() << "TCPSocketUtil::sendToFileFriend " << "无法获取指定伙伴的ip地址 " << friendId << endl;
        return false;
    }

    if (msg.getMsgType() != TCPCtrlMsgType::TASKEXECUING) {
        qDebug() << "TCPSocketUtil::sendToFileFriend " << "向朋友客户端发送的消息类型不合法 " << friendId << endl;
        return false;
    }

    this->fileGuests[friendId]->write(msg.toMsg());
    return true;
}

bool TCPSocketUtil::failToHelpFileFriend(QAbstractSocket::SocketError error, qint32 friendId)
{
    qDebug() << "TCPSocketUtil::failToHelpFileFriend " << "无法和伙伴客户端建立稳定连接 " << friendId << endl;
    qDebug() << "TCPSocketUtil::failToHelpFileFriend " << this->fileGuests[friendId]->errorString() << endl;
    qDebug() << "TCPSocketUtil::failToHelpFileFriend " << "SocketError " << error << endl;

    this->fileGuests[friendId]->close();
    delete this->fileGuests[friendId];
    this->fileGuests[friendId] = nullptr;
    this->fileGuests.remove(friendId);

    return true;
}
