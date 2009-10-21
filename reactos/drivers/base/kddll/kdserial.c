/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kdserial.c
 * PURPOSE:         Serial communication functions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@ewactos.org)
 */

#include "kddll.h"



/******************************************************************************
 * \name KdpSendBuffer
 * \brief Sends a buffer of data to the serial KD port.
 * \param Buffer Pointer to the data.
 * \param Size Size of data in bytes.
 */
VOID
NTAPI
KdpSendBuffer(
    IN PVOID Buffer,
    IN ULONG Size)
{
    INT i;
    for (i = 0; i < Size; i++)
    {
        KdpSendByte(((PUCHAR)Buffer)[i]);
    }
}

/******************************************************************************
 * \name KdpReceiveBuffer
 * \brief Recieves data from the KD port and fills a buffer.
 * \param Buffer Pointer to a buffer that receives the data.
 * \param Size Size of data to receive in bytes.
 * \return KDP_PACKET_RECEIVED if successful. 
 *         KDP_PACKET_TIMEOUT if the receice timed out.
 */
KDP_STATUS
NTAPI
KdpReceiveBuffer(
    OUT PVOID Buffer,
    IN  ULONG Size)
{
    ULONG i;
    PUCHAR ByteBuffer = Buffer;
    KDP_STATUS Status;

    for (i = 0; i < Size; i++)
    {
        /* Try to get a byte from the port */
        Status = KdpReceiveByte(&ByteBuffer[i]);

        if (Status != KDP_PACKET_RECEIVED)
        {
            return Status;
        }
    }

    return KDP_PACKET_RECEIVED;
}


/******************************************************************************
 * \name KdpReceivePacketLeader
 * \brief Recieves a packet leadr from the KD port.
 * \param PacketLeader Pointer to an ULONG that receives the packet leader.
 * \return KDP_PACKET_RECEIVED if successful. 
 *         KDP_PACKET_TIMEOUT if the receice timed out.
 */
KDP_STATUS
NTAPI
KdpReceivePacketLeader(
    OUT PULONG PacketLeader)
{
    UCHAR Index = 0, Byte, Buffer[4];
    KDP_STATUS KdStatus;

    /* Set first character to 0 */
    Buffer[0] = 0;

KdpDbgPrint("KdpReceivePacketLeader\n" );

    do
    {
        /* Receive a single Byte */
        KdStatus = KdpReceiveByte(&Byte);

        /* Check for timeout */
        if (KdStatus == KDP_PACKET_TIMEOUT)
        {
            /* Report timeout */
            KdpDbgPrint("KDP_PACKET_TIMEOUT\n");
            return KDP_PACKET_TIMEOUT;
        }

        /* Check if we received a byte */
        if (KdStatus == KDP_PACKET_RECEIVED)
        {
            /* Check for breakin byte */
            if (Byte == BREAKIN_PACKET_BYTE)
            {
                KdpDbgPrint("BREAKIN_PACKET_BYTE\n");
            }

            /* Check if this is a valid packet leader byte */
            if (Byte == PACKET_LEADER_BYTE ||
                Byte == CONTROL_PACKET_LEADER_BYTE)
            {
                KdpDbgPrint("received byte 0x%x, Index = %d\n", Byte, Index);
                /* Check if we match the first byte */
                if (Byte != Buffer[0])
                {
                    /* No, this is the new byte 0! */
                    Index = 0;
                }

                /* Store the byte in the buffer */
                Buffer[Index] = Byte;

                /* Continue with next byte */
                Index++;
                continue;
            }
        }

        /* Restart */
        Index = 0;
        Buffer[0] = 0;
    }
    while (Index < 4);

    /* Enable the debugger */
//    KdDebuggerNotPresent = FALSE;
    SharedUserData->KdDebuggerEnabled |= 0x00000002;

    /* Return the received packet leader */
    *PacketLeader = *(PULONG)Buffer;

KdpDbgPrint("KDP_PACKET_RECEIVED\n");

    return KDP_PACKET_RECEIVED;
}

