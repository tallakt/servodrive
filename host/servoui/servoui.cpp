/*
 * Copyright (C) Chris Desjardins 2008 - code@chrisd.info
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "servosoc.h"

#undef  SERVOUI_DEBUG
#define SERVOUI_MAX_SERVO  SERVOSOC_SERVO_B
class CServoUI
{
public:
    CServoUI(char *szJoyStickName, char *szIpAddr);
    ~CServoUI();
    void Drive();

protected:
    void SetupSocket(char *szIpAddr);

    bool ProcessEvent(js_event jsEvent);
    void UpdateServoList(js_event jsEvent);

    int m_fdJoyStick;
    int m_nSocket;
    sockaddr_in m_addrDevice;
    TServoData m_sServoList[SERVOSOC_MAX_SERVOS];
};

int ServoCmp(const void * a, const void * b)
{
    TServoData *psa;
    TServoData *psb;
    psa = (TServoData*)a;
    psb = (TServoData*)b;
    return (psa->m_nJoyValue - psb->m_nJoyValue);
}

CServoUI::CServoUI(char *szJoyStickName, char *szIpAddr)
{
    int i;

    printf("Attempting to read joy stick: %s\n", szJoyStickName);
    m_fdJoyStick = open(szJoyStickName, O_RDONLY);
    if (m_fdJoyStick < 0)
    {
        printf("Error: Unable to open %s\n", szJoyStickName);
    }
    /* init axis... even tho it will be done again... */
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++)
    {
        m_sServoList[i].m_nJoyValue = 0;
        if (i <= SERVOUI_MAX_SERVO)
        {
            m_sServoList[i].m_nServoNumber = i;
        }
        else
        {
            m_sServoList[i].m_nServoNumber = SERVOSOC_SERVO_INVALID;
        }
    }
    SetupSocket(szIpAddr);
}

CServoUI::~CServoUI()
{
    if (m_fdJoyStick > 0)
    {
        close(m_fdJoyStick);
    }
    if (m_nSocket >= 0)
    {
        close(m_nSocket);
    }
}

void CServoUI::SetupSocket(char *szIpAddr)
{
    m_nSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_nSocket < 0)
    {
        printf("ERROR: Unable to open socket\n");
    }
    else
    {
        memset(&m_addrDevice, 0, sizeof(m_addrDevice));
        m_addrDevice.sin_family = AF_INET;
        m_addrDevice.sin_addr.s_addr = inet_addr(szIpAddr);
        m_addrDevice.sin_port = htons(SERVOSOC_PORT);
    }
}

void CServoUI::UpdateServoList(js_event jsEvent)
{
    int i;
    int nServos = 0;
    for (i = 0; i < SERVOSOC_MAX_SERVOS; i++)
    {
        if (m_sServoList[i].m_nServoNumber == SERVOSOC_SERVO_INVALID)
        {
            break;
        }
        if (m_sServoList[i].m_nServoNumber == jsEvent.number)
        {
            m_sServoList[i].m_nJoyValue = jsEvent.value;
        }
        nServos++;
    }
    qsort(m_sServoList, nServos, sizeof(TServoData), ServoCmp);
}

bool CServoUI::ProcessEvent(js_event jsEvent)
{
    bool ret = true;
    if (jsEvent.type & JS_EVENT_AXIS)
    {
        if (jsEvent.number <= SERVOSOC_SERVO_B)
        {
            UpdateServoList(jsEvent);
            if (sendto(m_nSocket, m_sServoList, sizeof(m_sServoList), 0, (sockaddr*)&m_addrDevice, sizeof(m_addrDevice)) != sizeof(m_sServoList))
            {
                printf("ERROR: sendto failed!\n");
                ret = false;
            }
        }
    }
    return ret;
}

void CServoUI::Drive()
{
    js_event jsEvent;
    if ((m_fdJoyStick > 0) && (m_nSocket > 0))
    {
        while (1)
        {
            read(m_fdJoyStick, &jsEvent, sizeof(js_event));
#ifdef SERVOUI_DEBUG
            printf("time:%i value:%i type:%x number:%i\n", 
                jsEvent.time, jsEvent.value, jsEvent.type, jsEvent.number);
#endif
            if (ProcessEvent(jsEvent) == false)
            {
                break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    CServoUI *pServoUI;

    if (argc != 3)
    {
        printf("Joy stick device name needed, and target IP addess\n");
    }
    else
    {
        pServoUI = new CServoUI(argv[1], argv[2]);
        if (pServoUI == NULL)
        {
            printf("ERROR: Unable to allocate memory for CServoUI\n");
        }
        else
        {
            pServoUI->Drive();
        }
    }
    return 0;
}


