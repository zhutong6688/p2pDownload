#include "msgutil.h"

/**
 * @brief The MsgUtil class
 * @author 余宗宪
 * 生成控制信息的工具类
 */

MsgUtil::MsgUtil()
{

}

CtrlMsg MsgUtil::createLoginMsg(QString hostName, QString pwd, quint16 port, quint16 filePort)
{
    CtrlMsg loginMsg(UDPCtrlMsgType::LOGIN);
    loginMsg.setHostName(hostName);
    loginMsg.setPwd(pwd);
    loginMsg.setPort(port);
    loginMsg.setFilePort(filePort);

    return loginMsg;
}

CtrlMsg MsgUtil::createLogoutMsg(QString hostName, QString pwd)
{
    CtrlMsg logoutMsg(UDPCtrlMsgType::LOGOUT);
    logoutMsg.setHostName(hostName);
    logoutMsg.setPwd(pwd);

    return logoutMsg;
}

CtrlMsg MsgUtil::createObtainAllPartners(QString hostName, QString pwd)
{
    CtrlMsg obtainMsg(UDPCtrlMsgType::OBTAINALLPARTNERS);
    obtainMsg.setHostName(hostName);
    obtainMsg.setPwd(pwd);

    return obtainMsg;
}

CtrlMsg MsgUtil::createP2PTrans(QString hostName, QString pwd, QString partnerName)
{
    CtrlMsg transMsg(UDPCtrlMsgType::P2PTRANS);
    transMsg.setHostName(hostName);
    transMsg.setPwd(pwd);
    transMsg.setPartnerName(partnerName);

    return transMsg;
}

CommMsg MsgUtil::createP2PPunchMsg()
{
    CommMsg punchMsg(TCPCtrlMsgType::P2PPUNCH);

    return punchMsg;
}

CommMsg createAreYouAliveMsg()
{
    CommMsg youAliveMsg(TCPCtrlMsgType::AREYOUALIVE);

    return youAliveMsg;
}


CommMsg MsgUtil::createAreYouAliveMsg()
{
    CommMsg aliveMsg(TCPCtrlMsgType::AREYOUALIVE);

    return aliveMsg;
}

CommMsg MsgUtil::createIsAliveMsg(double rate)
{
    CommMsg aliveMsg(TCPCtrlMsgType::ISALIVE, rate);

    return aliveMsg;
}

CommMsg MsgUtil::createAskForHelpMsg(QString downloadAddress, qint32 lenMax)
{
    CommMsg downloadMsg(TCPCtrlMsgType::ASKFORHELP, downloadAddress, lenMax);

    return downloadMsg;
}

CommMsg MsgUtil::creteAgreeToHelpMsg()
{
    CommMsg agreeMsg(TCPCtrlMsgType::AGREETOHELP);

    return agreeMsg;
}

CommMsg MsgUtil::creteRefuseToHelpMsg()
{
    CommMsg refuseMsg(TCPCtrlMsgType::REFUSETOHELP);

    return refuseMsg;
}

CommMsg MsgUtil::createDownloadTaskMsg(qint32 token, qint64 pos, qint32 len)
{
    CommMsg taskMsg(TCPCtrlMsgType::DOWNLOADTASK, token, pos, len);

    return taskMsg;
}

CommMsg MsgUtil::createTaskFinishMsg(qint32 token)
{
    CommMsg finishMsg(TCPCtrlMsgType::TASKFINISH);
    finishMsg.setToken(token);

    return finishMsg;
}

CommMsg MsgUtil::creteTaskFailureMsg(qint32 token)
{
    CommMsg failureMsg(TCPCtrlMsgType::TASKFAILURE);
    failureMsg.setToken(token);

    return failureMsg;
}

CommMsg MsgUtil::createThankYourHelpMsg(qint32 token, qint32 index)
{
    CommMsg thankMsg(TCPCtrlMsgType::THANKYOURHELP, token, index);

    return thankMsg;
}

CommMsg MsgUtil::createEndYourHelpMsg()
{
    CommMsg endMsg(TCPCtrlMsgType::ENDYOURHELP);
    return endMsg;
}

FileMsg MsgUtil::createTaskExecuingMsg(qint32 token, qint32 index, qint8 lastOne, QByteArray & msg)
{
    FileMsg execMsg(TCPCtrlMsgType::TASKEXECUING, token, index, lastOne);
    execMsg.setMsg(msg);

    return execMsg;
}
