12.6. DMA
The RP2350 Direct Memory Access (DMA) controller performs bulk data transfers on a processor’s behalf. This leaves
processors free to attend to other tasks or enter low-power sleep states. The DMA dual bus manager ports can issue
one read and one write access per cycle. The data throughput is therefore far greater than one of RP2350’s processors.
RP2350 Datasheet
12.6. DMA 1094
Control/Status
Registers
Read Address FIFO
Write Address FIFO
Address Generator
AHB5
Read Manager
From
System
Transfer Data FIFO
AHB5
Write Manager
To
System
AHB5
Subordinate
Interface
Figure 122. DMA
Architecture Overview.
The read manager can
read data from some
address every clock
cycle. Likewise, the
write manager can
write to another
address. The address
generator produces
matched pairs of read
and write addresses,
which the managers
consume through the
address FIFOs. The
DMA can run up to 16
transfer sequences
simultaneously,
supervised by
software via the
control and status
registers.
The DMA can perform one read access and one write access, up to 32 bits in size, every clock cycle. There are 16
independent channels, each of which supervises a sequence of bus transfers in one of the following scenarios:
Memory-to-peripheral
a peripheral signals the DMA when it needs more data to transmit. The DMA reads data from an array in RAM or
flash, and writes to the peripheral’s data FIFO.
Peripheral-to-memory
a peripheral signals the DMA when it has received data. The DMA reads this data from the peripheral’s data FIFO,
and writes it to an array in RAM.
Memory-to-memory
the DMA transfers data between two buffers in RAM, as fast as possible.
Each channel has its own control and status registers (CSRs) that software can use to program and monitor the
channel’s progress. When multiple channels are active at the same time, the DMA shares bandwidth evenly between the
channels, with round-robin over all channels that are currently requesting data transfers.
The transfer size can be either 32, 16, or 8 bits. This is configured once for each channel: source transfer size and
destination transfer size are the same. The DMA performs byte lane replication on narrow writes, so byte data is
available in all 4 bytes of the databus, and halfword data in both halfwords.
Channels can be combined in varied ways for more sophisticated behaviour and greater autonomy. For example, one
channel can configure another, loading configuration data from a sequence of control blocks in memory, and the
second can then call back to the first via the CHAIN_TO option when it needs to be reconfigured.
Making the DMA more autonomous means that much less processor supervision is required: overall this allows the
system to do more at once, or to dissipate less power.
12.6.1. Changes from RP2040
The following new features have been added:
• Increased the number of DMA channels from 12 to 16.
• Increased the number of shared IRQ outputs from 2 to 4.
• Channels can be assigned to security domains using SECCFG_CH0 through SECCFG_CH15.
• The DMA now filters bus accesses using the built-in memory protection unit (Section 12.6.6.3).
• Interrupts can be assigned to security domains using SECCFG_IRQ0 through SECCFG_IRQ3.
• Pacing timers and the CRC sniffer can be assigned to security domains using the SECCFG_MISC register.
• The four most-significant bits of TRANS_COUNT (CH0_TRANS_COUNT) are redefined as the MODE field, which defines
what happens when TRANS_COUNT reaches zero:
RP2350 Datasheet
12.6. DMA 1095
◦
This backward-incompatible change reduces the maximum transfers in one sequence from 232-1 to 228-1.
◦ Mode 0x0 has the same behaviour as RP2040, so there is no need to modify software that performs less than
256 million transfers at a time.
◦ Mode 0x1, "trigger self", allows a channel to automatically restart itself after finishing a transfer sequence, in
addition to the usual end-of-sequence actions like raising an interrupt or triggering other channels. This can
be used for example to get periodic interrupts from streaming ring buffer transfers.
◦ Mode 0xf, "endless", allows a channel to run forever: TRANS_COUNT does not decrement.
• New CH0_CTRL_TRIG.INCR_READ_REV and CH0_CTRL_TRIG.INCR_WRITE_REV fields allow addresses to
decrement rather than increment, or to increment by two.
◦
Some existing fields in the CTRL registers, such as CH0_CTRL_TRIG.BUSY, have moved to accommodate the
new fields.
Some existing behaviour has been refined:
• The logic that adjusts values read from WRITE_ADDR and READ_ADDR according to the number of in-flight transfers is
disabled for address-wrapping and non-incrementing transfers (erratum RP2040-E12).
• You can now poll the ABORT register to wait for completion of an aborted channel (erratum RP2040-E13).
• DMA completion actions such as CHAIN_TO are now strictly ordered against the last write completion, so a CHAIN_TO
on a channel whose registers you write to is a well-defined operation.
◦
This enables the use of control blocks that don’t include one of the four trigger register aliases.
◦
Previously, a channel was considered to complete on the first cycle of its last write’s data phase. Now, a
channel is considered to complete on the last cycle of its last write’s data phase. This is usually the same
cycle, but it can be later when the DMA encounters a write data-phase bus stall.
• Previously, the DMA’s internal arbitration logic inserted an idle cycle after completing a round of active high-priority
channels (CH0_CTRL_TRIG.HIGH_PRIORITY), even if there were no active low-priority requests. This reduced DMA
throughput when lightly loaded. This idle cycle has been removed, eliminating lost throughput.
• IRQ assertion latency has been reduced by one cycle.
12.6.2. Configuring channels
Each channel has four control/status registers:
• READ_ADDR (CH0_READ_ADDR) is the address of the next memory location to read.
• WRITE_ADDR (CH0_WRITE_ADDR) is the address of the next memory location to write.
• TRANS_COUNT (CH0_TRANS_COUNT) shows the number of transfers remaining in the current transfer sequence and
programs the number of transfers in the next transfer sequence (see Section 12.6.2.2).
• CTRL (CH0_CTRL_TRIG) configures all other aspects of the channel’s behaviour, enables/disables the channel, and
provides completion status.
To directly instruct the DMA channel to perform a data transfer, software writes to these four registers, and then
triggers the channel (Section 12.6.3). To make the DMA more autonomous, you can also program one DMA channel to
write to another channel’s configuration registers, queueing up many transfer sequences in advance.
All four are live registers; they update their status continuously as the channel progresses.
12.6.2.1. Read and write addresses
READ_ADDR and WRITE_ADDR contain the address the channel will next read from, and write to, respectively. These registers
update automatically after each read/write access, incrementing to the next read/write address as required. The size of
the increment varies according to:
RP2350 Datasheet
12.6. DMA 1096
• the transfer size: 1, 2 or 4 byte bus accesses as per CH0_CTRL_TRIG.DATA_SIZE
• the increment enable for each address register: CH0_CTRL_TRIG.INCR_READ and CH0_CTRL_TRIG.INCR_WRITE
• the increment direction: CH0_CTRL_TRIG.INCR_READ_REV and CH0_CTRL_TRIG.INCR_WRITE_REV
Software should generally program these registers with new start addresses each time a new transfer sequence starts.
If READ_ADDR and WRITE_ADDR are not reprogrammed, the DMA will use the current values as start addresses for the next
transfer. For example:
• If the address does not increment (e.g. it is the address of a peripheral FIFO), and the next transfer sequence is
to/from that same address, there is no need to write to the register again.
• When transferring to/from a consecutive series of buffers in memory (e.g. scattering and gathering), an address
register will already have incremented to the start of the next buffer at the completion of a transfer.
By not programming all four CSRs for each transfer sequence, software can use shorter interrupt handlers, and more
compact control block formats when used with channel chaining (see register aliases in Section 12.6.3.1, chaining in
Section 12.6.3.2).
12.6.2.1.1. Address alignment
READ_ADDR and WRITE_ADDR must be aligned to the transfer size, specified in CH0_CTRL_TRIG.DATA_SIZE. For 32-bit
transfers, the address must be a multiple of four, and for 16-bit transfers, the address must be a multiple of two.
Software is responsible for correctly aligning addresses written to READ_ADDR and WRITE_ADDR: the DMA does not enforce
alignment.
If software initially writes a correctly aligned address, the address will remain correctly aligned throughout the transfer
sequence, because the DMA always increments READ_ADDR and WRITE_ADDR by a multiple of the transfer size. Specifically, it
increments by transfer size times -1, 0, 1 or 2, depending on the values of CH0_CTRL_TRIG.INCR_READ,
CH0_CTRL_TRIG.INCR_WRITE, CH0_CTRL_TRIG.INCR_READ_REV and CH0_CTRL_TRIG.INCR_WRITE_REV.
The DMA MPU and system-level bus security filters perform protection checks on the lowest byte address of all bytes
transferred on a given cycle (i.e. to the present value of READ_ADDR/WRITE_ADDR). RP2350 memory hardware ensures
unaligned bus accesses do not cause data to be read/written from the other side of a protection boundary. This means
that unaligned access can not be used to violate the memory protection model. Other than this, the result of an
unaligned access is unspecified.
12.6.2.2. Transfer count
Reading TRANS_COUNT (CH0_TRANS_COUNT) returns the number of transfers remaining in the current transfer sequence.
This value updates continuously as the channel progresses. Writing to TRANS_COUNT sets the length of the next transfer
sequence. Up to 228-1 transfers can be performed in one sequence (0x0fffffff, approximately 256 million).
Each time the channel starts a new transfer sequence, the most recent value written to TRANS_COUNT is copied to the live
transfer counter, which will then start to decrement again as the new transfer sequence makes progress. For debugging
purposes, the DBG_TCR (TRANS_COUNT reload value) registers display the last value written to each channel’s TRANS_COUNT.
If the channel is triggered multiple times without intervening writes to TRANS_COUNT, it performs the same number of
transfers each time. For example, when chained to, one channel might load a fixed-size control block into another
channel’s CSRs. TRANS_COUNT would be programmed once by software, and then reload automatically every time.
Alternatively, TRANS_COUNT can be written with a new value before starting each transfer sequence. If TRANS_COUNT is the
channel trigger (see Section 12.6.3.1), the channel will start immediately, and the value just written will be used, not the
value currently in the reload register.
RP2350 Datasheet
12.6. DMA 1097
 NOTE
The TRANS_COUNT is the number of transfers to be performed. The total number of bytes transferred is TRANS_COUNT
times the size of each transfer in bytes, given by CTRL.DATA_SIZE.
12.6.2.2.1. Count modes
The four most-significant bits of TRANS_COUNT contain the MODE field (CH0_TRANS_COUNT.MODE), which modifies the
counting behaviour of TRANS_COUNT. Mode 0x0 is the default: TRANS_COUNT decrements once for every bus transfer, and the
channel halts once TRANS_COUNT reaches zero and all in-flight transfers have finished. The value of 0x0 is chosen for
backward-compatibility with RP2040 software, which expects the TRANS_COUNT register to contain a 32-bit count rather
than a 4-bit mode and a 28-bit count. There are few use cases for a finite number of transfers greater than 228, which is
why the four most-significant bits have been reallocated for use with endless transfers.
Mode 0x1, TRIGGER_SELF, behaves the same as mode 0x0, except that rather than halting upon completion, the channel
immediately re-triggers itself. This is equivalent to a trigger performed by any other mechanism (Section 12.6.3):
TRANS_COUNT is reloaded, and the channel resumes from the current READ_ADDR and WRITE_ADDR addresses. A completion
interrupt is still raised (if CTRL.IRQ_QUIET is not set) and the specified CHAIN_TO operation is still performed. The main use
for this mode is streaming through SRAM ring buffers, where some action is required at regular intervals, for example
requesting the processor to refill an audio buffer once it is half-empty.
Mode 0xf, ENDLESS, disables the decrement of TRANS_COUNT. This means a channel will generally run indefinitely without
pause, though triggering a channel with a mode of 0xf and a count of 0x0 will result in the channel halting immediately.
All other values are reserved for future use and their effect is unspecified.
12.6.2.3. Control/Status
The CTRL register (CH0_CTRL_TRIG) has more, smaller fields than the other 3 registers. Among other things, CTRL is used
to:
• Configure the size of this channel’s data transfers through the DATA_SIZE field. Reads are always the same size as
writes.
• Configure if and how READ_ADDR and WRITE_ADDR increment after each read or write through the INCR_READ,
INCR_READ_REV, INCR_WRITE, INCR_WRITE_REV, RING_SEL, and RING_SIZE fields. Ring transfers are available, where one of the
address pointers wraps at some power-of-2 boundary.
• Select another channel (or none) to trigger when this channel completes through the CHAIN_TO field.
• Select a peripheral data request (DREQ) signal to pace this channel’s transfers, via the TREQ_SEL field.
• See when the channel is idle, using the BUSY flag.
• See if the channel has encountered a bus error in the READ_ERROR and WRITE_ERROR flags, or the combined error status
in the AHB_ERROR flag.
12.6.3. Triggering channels
After a channel has been correctly configured, you must trigger it. This instructs the channel to begin scheduling bus
accesses, either paced by a peripheral data request signal (DREQ) or as fast as possible. The following events can
trigger a channel:
• A write to a channel trigger register.
• Completion of another channel whose CHAIN_TO points to this channel.
• A write to the MULTI_CHAN_TRIGGER register (can trigger multiple channels at once).
Each trigger mechanism covers different use cases. For example, trigger registers are simple and efficient when
RP2350 Datasheet
12.6. DMA 1098
configuring and starting a channel in an interrupt service routine because the channel is triggered by the last
configuration write. CHAIN_TO allows one channel to callback to another channel, which can then reconfigure the first
channel. MULTI_CHAN_TRIGGER allows software to simply start a channel without touching any of its configuration
registers.
When triggered, the channel sets its CTRL.BUSY flag to indicate it is actively scheduling transfers. This remains set until
the transfer count reaches zero, or the channel is aborted via the CHAN_ABORT register (Section 12.6.8.3).
When a channel is already running, indicated by BUSY = 1, it ignores additional triggers. A channel that is disabled (CTRL.EN
is clear) also ignores triggers.
12.6.3.1. Aliases and triggers
Table 1145. Control
register aliases. Each
channel has four
control/status
registers. Each
register can be
accessed at multiple
different addresses. In
each naturally-aligned
group of four, all four
registers appear, in
different orders.
Offset +0x0 +0x4 +0x8 +0xc (Trigger)
0x00 (Alias 0) READ_ADDR WRITE_ADDR TRANS_COUNT CTRL_TRIG
0x10 (Alias 1) CTRL READ_ADDR WRITE_ADDR TRANS_COUNT_TRIG
0x20 (Alias 2) CTRL TRANS_COUNT READ_ADDR WRITE_ADDR_TRIG
0x30 (Alias 3) CTRL WRITE_ADDR TRANS_COUNT READ_ADD_TRIG
The four CSRs are aliased multiple times in memory. Each of the four aliases exposes the same four physical registers,
but in a different order. The final register in each alias (at offset +0xc, highlighted) is a trigger register. Writing to the
trigger register starts the channel.
Often, only alias 0 is used, and aliases 1 through 3 can be ignored. To configure and start the channel, write READ_ADDR,
WRITE_ADDR, TRANS_COUNT, and finally CTRL. Since CTRL is the trigger register in alias 0, this starts the channel.
The other aliases allow more compact control block lists when using one channel to configure another, and more
efficient reconfiguration and launch in interrupt handlers:
• Each CSR is a trigger register in one of the aliases:
◦ When gathering fixed-size buffers into a peripheral, the DMA channel can be configured and launched by
writing only READ_ADDR_TRIG.
◦ When scattering from a peripheral to fixed-size buffers, the channel can be configured and launched by
writing only WRITE_ADDR_TRIG.
• Useful combinations of registers appear as naturally-aligned tuples which contain a trigger register. In conjunction
with channel chaining and address wrapping, these implement compressed control block formats, e.g.:
◦
(WRITE_ADDR, TRANS_COUNT_TRIG) for peripheral scatter operations
◦
(TRANS_COUNT, READ_ADDR_TRIG) for peripheral gather operations, or calculating CRCs on a list of buffers
◦
(READ_ADDR, WRITE_ADDR_TRIG) for manipulating fixed-size buffers in memory
Trigger registers do not start the channel if:
• The channel is disabled via CTRL.EN (if the trigger is CTRL, the just-written value of EN is used, not the value currently
in the CTRL register)
• The channel is already running
• The value 0 is written to the trigger register (useful for ending control block chains, see null triggers (Section
12.6.3.3))
• The bus access has a security level lower than the channel’s security level (Section 12.6.6.1)
RP2350 Datasheet
12.6. DMA 1099
12.6.3.2. Chaining
When a channel completes, it can name a different channel to immediately be triggered. This can be used as a callback
for the second channel to reconfigure and restart the first.
This feature is configured through the CHAIN_TO field in the channel CTRL register. This 4-bit value selects a channel that
will start when this one finishes. A channel cannot chain to itself. Setting CHAIN_TO to a channel’s own index prevents
chaining.
Chain triggers behave the same as triggers from other sources, such as trigger registers. For example, they cause
TRANS_COUNT to reload, and they are ignored if the targeted channel is already running.
One application for CHAIN_TO is for a channel to request reconfiguration by another channel from a sequence of control
blocks in memory. Channel A is configured to perform a wrapped transfer from memory to channel B’s control registers
(including a trigger register), and channel B is configured to chain back to channel A when it completes each transfer
sequence. This is shown explicitly in the DMA control blocks example (Section 12.6.9.2).
Use of the register aliases (Section 12.6.3.1) enables compact formats for DMA control blocks: as little as one word, in
some cases.
Another use of chaining is a ping-pong configuration, where two channels each trigger one another. The processor can
respond to the channel completion interrupts and reconfigure each channel after it completes. However, the chained
channel, which has already been configured, starts immediately. In other words, channel configuration and channel
operation are pipelined. This can improve performance dramatically when a usage pattern requires many short transfer
sequences.
The Section 12.6.9 goes into more detail on the possibilities of chain triggers in the real world.
12.6.3.3. Null triggers and chain interrupts
As mentioned in Section 12.6.3.1, writing all-zeroes to a trigger register does not start the channel. This is called a null
trigger, and it has two purposes:
• Cause a halt at the end of an array of control blocks, by appending an all-zeroes block.
• Reduce the number of interrupts generated when using control blocks.
By default, channels generate an interrupt each time they finish a transfer sequence, unless that channel’s IRQ is
masked in INTE0 through INTE3. The rate of interrupts can be excessive, particularly as processor attention is generally
not required while a sequence of control blocks are in progress. However, processor attention is required at the end of a
chain.
The channel CTRL register has a field called IRQ_QUIET. Its default value is 0. When this set to 1, channels generate an
interrupt when they receive a null trigger, but not on normal completion of a transfer sequence. The interrupt is
generated by the channel that receives the trigger.
12.6.4. Data request (DREQ)
Peripherals produce or consume data at their own pace. If the DMA transferred data as fast as possible, loss or
corruption of data would ensue. DREQs are a communication channel between peripherals and the DMA that enables
the DMA to pace transfers according to the needs of the peripheral.
The CTRL.TREQ_SEL (transfer request) field selects an external DREQ. It can also be used to select one of the internal
pacing timers, or select no TREQ at all (the transfer proceeds as fast as possible), e.g. for memory-to-memory transfers.
12.6.4.1. System DREQ table
DREQ numbers use the following global assignment to peripheral DREQ channels:
RP2350 Datasheet
12.6. DMA 1100
Table 1146. DREQs DREQ DREQ Channel DREQ DREQ Channel DREQ DREQ Channel DREQ DREQ Channel
0 DREQ_PIO0_TX0 14 DREQ_PIO1_RX2 28 DREQ_UART0_TX 42 DREQ_PWM_WRAP10
1 DREQ_PIO0_TX1 15 DREQ_PIO1_RX3 29 DREQ_UART0_RX 43 DREQ_PWM_WRAP11
2 DREQ_PIO0_TX2 16 DREQ_PIO2_TX0 30 DREQ_UART1_TX 44 DREQ_I2C0_TX
3 DREQ_PIO0_TX3 17 DREQ_PIO2_TX1 31 DREQ_UART1_RX 45 DREQ_I2C0_RX
4 DREQ_PIO0_RX0 18 DREQ_PIO2_TX2 32 DREQ_PWM_WRAP0 46 DREQ_I2C1_TX
5 DREQ_PIO0_RX1 19 DREQ_PIO2_TX3 33 DREQ_PWM_WRAP1 47 DREQ_I2C1_RX
6 DREQ_PIO0_RX2 20 DREQ_PIO2_RX0 34 DREQ_PWM_WRAP2 48 DREQ_ADC
7 DREQ_PIO0_RX3 21 DREQ_PIO2_RX1 35 DREQ_PWM_WRAP3 49 DREQ_XIP_STREAM
8 DREQ_PIO1_TX0 22 DREQ_PIO2_RX2 36 DREQ_PWM_WRAP4 50 DREQ_XIP_QMITX
9 DREQ_PIO1_TX1 23 DREQ_PIO2_RX3 37 DREQ_PWM_WRAP5 51 DREQ_XIP_QMIRX
10 DREQ_PIO1_TX2 24 DREQ_SPI0_TX 38 DREQ_PWM_WRAP6 52 DREQ_HSTX
11 DREQ_PIO1_TX3 25 DREQ_SPI0_RX 39 DREQ_PWM_WRAP7 53 DREQ_CORESIGHT
12 DREQ_PIO1_RX0 26 DREQ_SPI1_TX 40 DREQ_PWM_WRAP8 54 DREQ_SHA256
13 DREQ_PIO1_RX1 27 DREQ_SPI1_RX 41 DREQ_PWM_WRAP9
12.6.4.2. Credit-based DREQ Scheme
The RP2350 DMA is designed for systems where:
• The area and power cost of large peripheral data FIFOs is prohibitive.
• The bandwidth demands of individual peripherals can be high, for example, >50% bus injection rate for short
periods.
• Bus latency is low, but multiple managers can compete for bus access.
In addition, the DMA’s transfer FIFOs and dual-manager-port structure permit multiple accesses to the same peripheral
to be in-flight at once to improve throughput. Choice of DREQ mechanism is therefore critical:
• The traditional "turn on the tap" method can cause overflow if multiple writes are backed up in the TDF. Some
systems solve this by over-provisioning peripheral FIFOs and setting the DREQ threshold below the full level at the
expense of precious area and power.
• The Arm-style single and burst handshake does not permit additional requests to be registered while the current
request is being served. This limits performance when FIFOs are very shallow.
The RP2350 DMA uses a credit-based DREQ mechanism. For each peripheral, the DMA attempts to keep as many
transfers in-flight as the peripheral has capacity for. This enables full bus throughput (1 word per clock) through an 8-
deep peripheral FIFO with no possibility of overflow or underflow in the absence of fabric latency or contention.
For each channel, the DMA maintains a counter. Each 1-clock pulse on the dreq signal increments this counter. When
non-zero, the channel requests a transfer from the DMA’s internal arbiter. The counter decrements when the transfer is
issued to the address FIFOs. At this point the transfer is in flight, but has not yet necessarily completed.
The counter is saturating, and six bits in size. The counter ignores increments at the maximum value or decrements at
zero. The six-bit counter size supports counts up to the depth of any FIFO on RP2350.
RP2350 Datasheet
12.6. DMA 1101
clk
0 1 0 1 2
dreq
chan count
chan issue
1
Figure 123. DREQ
counting
The effect is to upper bound the number of in-flight transfers based on the amount of room or data available in the
peripheral FIFO. In the steady state, this gives maximum throughput, but can’t underflow or underflow. This approach
has the following caveats:
• The user must not access a FIFO currently being serviced by the DMA. This causes the channel and peripheral to
become desynchronised, and can cause corruption or loss of data.
• Multiple channels must not be connected to the same DREQ.
12.6.5. Interrupts
Each channel can generate interrupts; these can be masked on a per-channel basis using one of the four identical
interrupt enable registers, INTE0 through INTE3. There are three circumstances where a channel raises an interrupt
request:
• On the completion of each transfer sequence, if CTRL.IRQ_QUIET is disabled
• On receiving a null trigger, if CTRL.IRQ_QUIET is enabled
• On a read or write bus error
The masked interrupt status is visible in the INTS registers; there is one bit for each channel. Interrupts are cleared by
writing a bit mask to INTS. One idiom for acknowledging interrupts is to read INTS, then write the same value back, so
only enabled interrupts are cleared.
The RP2350 DMA provides four system IRQs, with independent masking and status registers (e.g. INTE0, INTE1). Any
combination of channel interrupt requests can be routed to each system IRQ, though generally software only routes
each channel interrupt to a single system IRQ. For example:
• Some channels can be given a higher priority in the system interrupt controller, if they have particularly tight timing
requirements.
• In multiprocessor systems, different channel interrupts can be routed independently to different cores.
• When channels are assigned to a mixture of security domains, IRQs can also be assigned, so that software in each
security domain can get interrupts from its own channels.
For debugging purposes, the INTF registers can force any channel interrupt to be asserted, which will cause assertion of
any system IRQs that have that channel interrupt’s enable bit set in their respective INTE registers.
12.6.6. Security
RP2350’s processors support partitioning of memory and peripherals into multiple security domains. This partitioning is
extended into the DMA, so that different security contexts can safely use their assigned channels without breaking any
of the security invariants laid out by the processor security model. For example, an Arm processor in the Non-secure
state must not be able to use the DMA to access memory or peripherals owned by Secure software.
The DMA defines four security levels that map onto Arm or RISC-V processor security states:
• 3: SP (secure and privileged)
◦
Equivalent to Arm processors in the Secure, Privileged state
◦
Equivalent to RISC-V processors in Machine mode
• 2: SU (secure and unprivileged)
RP2350 Datasheet
12.6. DMA 1102
◦
Equivalent to Arm processors in the Secure, Normal state
• 1: NSP (nonsecure and privileged)
◦
Equivalent to Arm processors in the Non-secure, Privileged state
◦
Equivalent to RISC-V processors in Supervisor mode
• 0: NSU (nonsecure and unprivileged)
◦
Equivalent to Arm processors in the Non-secure, Normal state
◦
Equivalent to RISC-V processors in User mode
So that the DMA can compare different security levels in a consistent way, they are considered ordered, with SP > SU >
NSP > NSU. For example, when we say that a channel requires a minimum of SU to access its registers, this means that
SP and SU are acceptable, and NSP and NSU are not. As a rule, every action has a reaction that is at or below the
security level of the original action, and so the DMA can not be used to escalate accesses to a higher security level.
Software assigns internal DMA resources, like channels, interrupts, pacing timers and the CRC sniffer, to one of the four
possible security levels. These resources are then accessible only at and above that level. Channel assignment in
particular is discussed in Section 12.6.6.1.
The DMA memory protection unit (Section 12.6.6.3) defines the minimum security level required to access up to eight
programmable address ranges, so that channels of a given security level can not access memory beyond their means.
This MPU is intended to mirror the SRAM and XIP memory protection boundaries configured in the processor SAU or
PMP. In addition to the internal filtering performed by the DMA MPU, accesses are filtered by the system bus according
to the ACCESSCTRL filter rules described in Section 10.6.2.
The combination of these features allows the DMA to be safely shared by software running in different security
domains. If this is not desired, the entire DMA block can instead be assigned wholesale to a single security domain
using the ACCESSCTRL DMA register.
12.6.6.1. Channel security assignment
Channels are assigned to security domains using the channel SECCFG registers, SECCFG_CH0 through SECCFG_CH15.
There is one register per channel. Each register contains a 2-bit security level, and a lock bit that prevents that SECCFG
register from being changed once configured. At reset, all channels are assigned to the SP security level, which is the
highest.
The security level of a channel defines:
• The security level of bus transfers performed by this channel, which is checked against both the DMA memory
protection unit and the ACCESSCTRL bus-level filters described in Section 10.6.2.
• The minimum security level required to read or write this channel’s registers; access from a lower level returns a
bus fault.
• The minimum security level that must be defined on a shared IRQ line for that IRQ to be able to observe this
channel’s interrupts (Section 12.6.6.2), or for this channel’s interrupt to be set/cleared through that IRQ’s registers.
• The minimum bus security level required to clear this channel’s interrupts through the INTR register.
• Which DREQs a channel can observe: channels assigned to the NSP or NSU security levels can not observe DREQs
of Secure-only peripherals (as defined by the ACCESSCTRL peripheral configuration).
• Which pacing timer TREQs can be observed; pacing timer security levels are configured by SECCFG_MISC and
must be no higher than the channel security level for the channel in order to observe the TREQ.
• Whether the channel is visible to the CRC sniffer; the sniffer’s security level is configured by SECCFG_MISC and
must be no lower than the observed channel’s security level.
• Which channels this channel can trigger with a CHAIN_TO; chaining from lower to higher security levels is not
permitted.
RP2350 Datasheet
12.6. DMA 1103
• The minimum bus security level required to trigger this channel with a write to MULTI_CHAN_TRIGGER.
The channel SECCFG registers require privileged writes (SP/NSP), and will generate a bus fault on an attempted
unprivileged write (SU/NSU). Additionally, the S bit (MSB of the security level) and the LOCK bit are writable only by SP,
whilst the P bit (LSB of the security level) is also writable by NSP, if and only if the S bit is clear. Reads are always
allowed: it is always possible to query which channels are assigned to you by reading the channel SECCFG registers.
Each channel SECCFG register can be locked manually by writing a one to the LOCK bit in that register, and will also lock
automatically upon a successful write to one of the channel’s control registers such as CH0_CTRL_TRIG. This
automatic locking avoids any race conditions that can arise from a channel’s security level changing after it has already
started making transfers, or from leaking secure pointers that have been written to its control registers. After a channel
SECCFG register has been locked, it becomes read-only. LOCK bits can be cleared only by a full reset of the DMA block.
SECCFG registers can be written multiple times before being locked, so the full assignment does not have to be known up
front: for example, Secure Arm software can set spare channels to NSP before launching the Non-secure software
context, and Non-secure, Privileged software can then set the remaining channels it does not need to NSU before
returning to the Non-secure, Normal context.
12.6.6.2. Interrupt Security Assignment
The RP2350 DMA has four system-level interrupt request lines (IRQs), each of which can be asserted on any
combination of channel interrupts, as defined by the channel masks in the interrupt enable registers INTE0 through
INTE3. Because the timing of interrupts can leak information, and because it is possible to cause software to
malfunction by deliberately manipulating its interrupts, access to the channel interrupt flags must be controlled.
The interrupt security configuration registers, SECCFG_IRQ0 through SECCFG_IRQ3, define the security level for each
interrupt. This is one of the four security levels laid out in Section 12.6.6. The security level of an IRQ defines:
• Which channels are visible in this IRQ’s status registers; channels of a level higher than the IRQ’s will read back as
zero.
• Whether a bus access to this IRQ’s control and status registers is permitted; bus accesses below this IRQ’s
security level will return bus faults and have no effect on the DMA.
• Which channels will assert this IRQ; channels of a level higher than this IRQ’s level will not cause the interrupt to
assert, even if relevant INTE bit is set.
• Whether a channel’s interrupt can be cleared through this IRQ’s INTS register, or set through this channel’s INTF
register; the interrupt flags of channels of higher security level than the IRQ can not be set or cleared.
The INTR register is shared between all IRQs, so it does not respect any of the IRQ security levels. Instead, it follows the
security level of the bus access: reads of INTR will return the interrupt flags of all channels at or below the security level
of the bus access (with higher-level channels reading back as zeroes), and writes to INTR have write-one-clear behaviour
on channels which are at or below the security level of the bus access.
12.6.6.3. Memory protection unit
The DMA memory protection unit (MPU) monitors the addresses of all read/write transfers performed by the DMA, and
notes the security level of the originating channel. The MPU is configured in advance with a user-defined security
address map, which specifies the minimum security level required to access up to eight dynamically configured regions.
This is one of the four security levels defined in Section 12.6.6.
Transfers that fail to meet the minimum security level for their address are shot down before reaching the system bus,
and a bus error is returned to the originating channel. This will be reported as either a read or write bus error in the
channel’s CTRL register, depending on whether it was a read or write address that failed the security check.
The intended use for the DMA MPU is to mirror the security definitions of SRAM and XIP memory from the processor
SAU or PMP. The number of DMA MPU regions is not sufficient for assigning individual peripherals, so the
ACCESSCTRL bus access registers (Section 10.6.2) are provided for this purpose.
RP2350 Datasheet
12.6. DMA 1104
Each of the eight MPU regions is configured with a base address, MPU_BAR0 through MPU_BAR7 for each region, and a
limit address, MPU_LAR0 through MPU_LAR7.
MPU regions have a granularity of 32 bytes, so the base/limit addresses are configured by the 27 most-significant bits
of each BAR/LAR register (bits 31:5). Addresses match MPU regions when the 27 most-significant bits of the address are
greater than or equal to the BAR address bits, and less than or equal to the LAR address bits. For example, when
MPU_BAR0 and MPU_LAR0 both have the value 0x10000000, MPU region 0 matches on a 32-byte region extending from
byte address 0x10000000 to 0x1000001f (inclusive). Regions can be enabled or disabled using the LAR.EN bits — if a region is
disabled, it matches no addresses.
The minimum security level required to access each region is defined by the S and P bits in the LSBs of that region’s LAR
register. When an address matches multiple regions, the lowest-numbered region applies. This matches the tie-break
rules for the RISC-V PMP, but is different from the Arm SAU tie-break rules, so care must be taken when mirroring SAU
mappings with overlapping regions. When none of the MPU regions are matched, the security level is defined by the
global MPU_CTRL.S and MPU_CTRL.P bits.
The MPU configuration registers (MPU_CTRL, MPU_BAR0 through MPU_BAR7 and MPU_LAR0 through MPU_LAR7) do
not permit unprivileged access. Bus accesses at the SU and NSU security levels will return a bus fault and have no other
effect.
The MPU registers are also mostly read-only to NSP accesses, with the sole exception being the region P bits which are
NSP-writable if and only if the corresponding region’s S bit is clear. This delegates to Privileged, Non-secure software
the decision of whether Non-secure regions are NSU-accessible.
12.6.7. Bus error handling
A bus error is an error condition flagged to one of the DMA’s manager ports in response to an attempted read or write
transfer, indicating the transfer was rejected for one of the following reasons:
• The DMA MPU forbids access to this address at the originating channel’s security level (Section 12.6.6.3).
• The bus fabric failed to decode the address; the address did not match any known memory location (for example
SIO is not visible from the DMA bus ports as it is tightly coupled to the processors).
• ACCESSCTRL forbids access to the addressed region at the originating channel’s privilege level (Section 10.6.2).
• ACCESSCTRL forbids DMA access to the addressed region, irrespective of privilege.
• The APB bridge returned a timeout fault for a transfer exceeding 65535 cycles (e.g. accessed ADC whilst clk_adc
was stopped).
• The downstream bus port returned an error response for any other device-specific reason, e.g. attempting to
access configuration registers for a DMA channel with higher security level (Section 12.6.6.1).
12.6.7.1. Response to bus errors
Upon encountering a bus error, the DMA halts the offending channel and reports the error through the channel’s
CH0_CTRL_TRIG.READ_ERROR and WRITE_ERROR flags. The channel stops scheduling bus accesses.
Bus errors are exceptional events which usually indicate misconfiguration of the DMA or some other system hardware.
Therefore the DMA refuses to restart the offending channel until its error status is cleared by writing 1 to the relevant
error flag. Other channels are not affected, and continue their transfer sequences uninterrupted.
A channel which encounters a bus error does not CHAIN_TO other channels.
Bus errors always cause the channel’s interrupt request to be asserted. Whether or not this causes a system-level IRQ
depends on the channel masks configured in interrupt enable registers INTE0 through INTE3.
RP2350 Datasheet
12.6. DMA 1105
12.6.7.2. Recovery after bus errors
If an error is reported through READ_ERR/WRITE_ERR then, before restarting the channel, software must:
1. Poll for a low BUSY status to ensure that all in-flight transfers for this channel have been flushed from the DMA’s bus
pipeline.
2. Clear the error flags by writing 1 to each flag.
Generally the BUSY flag will already be low long before the processor enters its interrupt handler and checks the error
status, but it is possible for these events to overlap when the DMA is accessing a slow device such as XIP with a high
SCK divisor and processors are executing from SRAM.
READ_ADDR and WRITE_ADDR contain the approximate address where the bus error was encountered. This can be useful for
the programmer to understand why the bus error occurred, and fix the software to avoid it in future.
Since the DMA performs reads and writes in parallel, it is possible for a channel to encounter both a read and write error
simultaneously, and in this case the DMA sets both READ_ERR and WRITE_ERR. You must clear both.
12.6.7.3. Halt timing
The DMA halts the channel as soon as possible following a bus error. This suppresses future reads and writes. Because
the request to access the bus is masked, the bus access has no side effects on the system. The timing relationships are
not straightforward due to the DMA’s pipelining and buffering. The DMA provides the following ordering guarantees
between transfers originating from one channel:
• Read error → read suppression: Any reads scheduled to occur after a faulting read will be suppressed, but can still
increment READ_ADDR up to two times total
• Write error → write suppression: Any writes scheduled to occur after a faulting write will be suppressed, but can
still increment WRITE_ADDR up to four times total
• Read error → write suppression:
◦
Any write paired with a faulting read will be suppressed, but will increment WRITE_ADDR
◦
Any write following the first write paired with a faulting read will be suppressed, but can increment WRITE_ADDR
up to three times total
◦
Up to three writes immediately preceding the first write paired with a faulting read can be suppressed, but will
increment WRITE_ADDR
• Write error → read suppression:
◦
Reads paired with writes before the first faulting write will not be suppressed, and will increment READ_ADDR.
◦
Up to two read transfers paired with writes after the first faulting write can be suppressed, and can increment
READ_ADDR
"Paired with" in the above paragraph refers to the write access which writes data originating from a particular read
transfer, or vice versa. The DMA always schedules read and write accesses in matched pairs.
Slight variability in halt behaviour is due to the buffering of in-flight transfers, and the parallel operation of the read and
write bus ports. The values of READ_ADDR/WRITE_ADDR following a bus error can be slightly beyond the address that
experienced the first error, but the difference is bounded, and usually this is still sufficient to diagnose the reason for the
fault. Additionally, READ_ADDR and WRITE_ADDR are guaranteed to over-increment by the same amount, since reads and
writes are always scheduled in pairs.
In addition to the increments mentioned above, READ_ADDR/WRITE_ADDR always point to the next address to be written, so
always point slightly past the faulting address if address increment is enabled.
RP2350 Datasheet
12.6. DMA 1106
12.6.8. Additional features
12.6.8.1. Pacing timers
These allow transfer of data roughly once every n clk_sys clocks instead of using external peripheral DREQ to trigger
transfers. A fractional (X/Y) divider is used, and will generate a maximum of 1 request per clk_sys cycle.
There are 4 timers available in RP2350. Each DMA channel is able to select any of these in CTRL.TREQ_SEL. There is one
register used to configure the pacing coefficients for each timer, TIMER0 through TIMER3.
Each timer’s security level is defined by a register field in SECCFG_MISC. This defines the minimum bus security level
required to configure that timer (lower levels will get a bus fault), and the minimum channel security level required to
observe that timer’s TREQ.
12.6.8.2. CRC calculation
The DMA can watch data from a given channel passing through the data FIFO, and calculate checksums based on this
data. This a purely passive affair: the data is not altered by this hardware, only observed.
The feature is controlled via the SNIFF_CTRL and SNIFF_DATA registers, and can be enabled/disabled per DMA transfer via
the CTRL.SNIFF_EN field.
As this hardware cannot place back-pressure on the FIFO, it must keep up with the DMA’s maximum transfer rate of 32
bits per clock.
The supported checksums are:
• CRC-32, MSB-first and LSB-first
• CRC-16-CCITT, MSB-first and LSB-first
• Simple summation (add to 32-bit accumulator)
• Even parity
The result register is both readable and writable, so that the initial seed value can be set.
Bit/byte manipulations are available on the result, which can aid specific use cases:
• Bit inversion
• Bit reversal
• Byte swap
These manipulations do not affect the CRC calculation, just how the data is presented in the result register.
The sniffer’s security level is configured by the SECCFG_MISC.SNIFF_S and SECCFG_MISC.SNIFF_P bits. This
determines the minimum bus security level required to access the sniffer’s control registers, as well as the maximum
channel security level that the sniffer can observe.
12.6.8.3. Channel abort
It is possible for a channel to get into an irrecoverable state. If commanded to transfer more data than a peripheral will
ever request, the channel will never complete. Clearing the CTRL.EN bit pauses the channel, but does not solve the
problem. This should not occur under normal circumstances, but it is important that there is a mechanism to recover
without simply hard-resetting the entire DMA block.
In such a situation, use the CHAN_ABORT register to force the channel to complete early. There is one bit for each
channel. Writing a 1 to the corresponding bit terminates the channel. This clears the transfer counter and forces the
channel into an inactive state.
RP2350 Datasheet
12.6. DMA 1107
At the time an abort is triggered, a channel might have bus transfers currently in flight between the read and write
manager. These transfers cannot be revoked. The CTRL.BUSY flag stays high until these transfers complete, and the
channel reaches a safe state. This generally takes only a few cycles. The channel must not be restarted until its
CTRL.BUSY flag de-asserts. Starting a new sequence of transfers whilst transfers from an old sequence are still in flight
will cause unpredictable behaviour.
The sequence to abort one or more channels in an unknown state (also accounting for the behaviour described in
RP2350-E5 is:
1. Clear the EN bit and disable CHAIN_TO for all channels to be aborted.
2. Write the CHAN_ABORT register with a bitmap of those same channels.
3. Poll the ABORT register until all bits set by the previous write are clear.
When aborting a channel involved in a CHAIN_TO, it is recommended to simultaneously abort all other channels involved in
the chain.
12.6.8.4. Debug
Debug registers are available for each DMA channel to show the dreq counter DBG_CTDREQ and next transfer count DBG_TCR.
These can also be used to reset a DMA channel if required.
12.6.9. Example use cases
12.6.9.1. Using interrupts to reconfigure a channel
When a channel finishes a block of transfers, it becomes available for making more transfers. Software detects that the
channel is no longer busy, and reconfigures and restarts the channel. One approach is to poll the CTRL_BUSY bit until the
channel is done, but this loses one of the key advantages of the DMA, namely that it does not have to operate in
lockstep with a processor. By setting the correct bit in INTE0 through INTE3, you can instruct the DMA to raise one of its
four interrupt request lines when a given channel completes. Rather than repeatedly asking if a channel is done, you are
told.
 NOTE
Having four system interrupt lines allows different channel completion interrupts to be routed to different cores, or
to pre-empt one another on the same core if one channel is more time-critical. It also allows channel interrupts to
target different security domains.
When the interrupt is asserted, the processor can be configured to drop whatever it is doing and call a user-specified
handler function. The handler can reconfigure and restart the channel. When the handler exits, the processor returns to
the interrupted code running in the foreground.
Pico Examples: https://github.com/raspberrypi/pico-examples/blob/master/dma/channel_irq/channel_irq.c Lines 35 - 52
35 void dma_handler() {
36 static int pwm_level = 0;
37 static uint32_t wavetable[N_PWM_LEVELS];
38 static bool first_run = true;
39 // Entry number `i` has `i` one bits and `(32 - i)` zero bits.
40 if (first_run) {
41 first_run = false;
42 for (int i = 0; i < N_PWM_LEVELS; ++i)
43 wavetable[i] = ~(~0u << i);
44 }
45
RP2350 Datasheet
12.6. DMA 1108
46 // Clear the interrupt request.
47 dma_hw->ints0 = 1u << dma_chan;
48 // Give the channel a new wave table entry to read from, and re-trigger it
49 dma_channel_set_read_addr(dma_chan, &wavetable[pwm_level], true);
50
51 pwm_level = (pwm_level + 1) % N_PWM_LEVELS;
52 }
In many cases, most of the configuration can be done the first time the channel starts. This way, only addresses and
transfer lengths need reprogramming in the interrupt handler.
Pico Examples: https://github.com/raspberrypi/pico-examples/blob/master/dma/channel_irq/channel_irq.c Lines 54 - 94
54 int main() {
55 #ifndef PICO_DEFAULT_LED_PIN
56 #warning dma/channel_irq example requires a board with a regular LED
57 #else
58 // Set up a PIO state machine to serialise our bits
59 uint offset = pio_add_program(pio0, &pio_serialiser_program);
60 pio_serialiser_program_init(pio0, 0, offset, PICO_DEFAULT_LED_PIN, PIO_SERIAL_CLKDIV);
61
62 // Configure a channel to write the same word (32 bits) repeatedly to PIO0
63 // SM0's TX FIFO, paced by the data request signal from that peripheral.
64 dma_chan = dma_claim_unused_channel(true);
65 dma_channel_config c = dma_channel_get_default_config(dma_chan);
66 channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
67 channel_config_set_read_increment(&c, false);
68 channel_config_set_dreq(&c, DREQ_PIO0_TX0);
69
70 dma_channel_configure(
71 dma_chan,
72 &c,
73 &pio0_hw->txf[0], // Write address (only need to set this once)
74 NULL, // Don't provide a read address yet
75 PWM_REPEAT_COUNT, // Write the same value many times, then halt and interrupt
76 false // Don't start yet
77 );
78
79 // Tell the DMA to raise IRQ line 0 when the channel finishes a block
80 dma_channel_set_irq0_enabled(dma_chan, true);
81
82 // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
83 irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
84 irq_set_enabled(DMA_IRQ_0, true);
85
86 // Manually call the handler once, to trigger the first transfer
87 dma_handler();
88
89 // Everything else from this point is interrupt-driven. The processor has
90 // time to sit and think about its early retirement -- maybe open a bakery?
91 while (true)
92 tight_loop_contents();
93 #endif
94 }
One disadvantage of this technique is that you don’t start to reconfigure the channel until some time after the channel
makes its last transfer. If there is heavy interrupt activity on the processor, this can be quite a long time, and quite a
large gap in transfers. This makes it difficult to sustain a high data throughput.
This is solved by using two channels, with their CHAIN_TO fields crossed over, so that channel A triggers channel B when it
completes, and vice versa. At any point in time, one of the channels is transferring data. The other is either already
RP2350 Datasheet
12.6. DMA 1109
configured to start the next transfer immediately when the current one finishes, or it is in the process of being
reconfigured. When channel A completes, it immediately starts the cued-up transfer on channel B. At the same time, the
interrupt is fired, and the handler reconfigures channel A so that it is ready when channel B completes.
12.6.9.2. DMA control blocks
Frequently, multiple smaller buffers must be gathered together and sent to the same peripheral. To address this use
case, the RP2350 DMA can execute a long and complex sequence of transfers without processor control. One channel
repeatedly reconfigures a second channel, and the second channel restarts the first each time it completes block of
transfers.
Because the first DMA channel transfers data directly from memory to the second channel’s control registers, the
format of the control blocks in memory must match those registers. Each time, the last register written to will be one of
the trigger registers (Section 12.6.3.1), which will start the second channel on its programmed block of transfers. The
register aliases (Section 12.6.3.1) give some flexibility for the block layout, and more importantly allow some registers
to be omitted from the blocks, so they occupy less memory and can be loaded more quickly.
This example shows how multiple buffers can be gathered and transferred to the UART, by reprogramming TRANS_COUNT
and READ_ADDR_TRIG:
Pico Examples: https://github.com/raspberrypi/pico-examples/blob/master/dma/control_blocks/control_blocks.c
  1 /**
  2 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
  3 *
  4 * SPDX-License-Identifier: BSD-3-Clause
  5 */
  6
  7 // Use two DMA channels to make a programmed sequence of data transfers to the
  8 // UART (a data gather operation). One channel is responsible for transferring
  9 // the actual data, the other repeatedly reprograms that channel.
 10
 11 #include <stdio.h>
 12 #include "pico/stdlib.h"
 13 #include "hardware/dma.h"
 14 #include "hardware/structs/uart.h"
 15
 16 // These buffers will be DMA'd to the UART, one after the other.
 17
 18 const char word0[] = "Transferring ";
 19 const char word1[] = "one ";
 20 const char word2[] = "word ";
 21 const char word3[] = "at ";
 22 const char word4[] = "a ";
 23 const char word5[] = "time.\n";
 24
 25 // Note the order of the fields here: it's important that the length is before
 26 // the read address, because the control channel is going to write to the last
 27 // two registers in alias 3 on the data channel:
 28 // +0x0 +0x4 +0x8 +0xC (Trigger)
 29 // Alias 0: READ_ADDR WRITE_ADDR TRANS_COUNT CTRL
 30 // Alias 1: CTRL READ_ADDR WRITE_ADDR TRANS_COUNT
 31 // Alias 2: CTRL TRANS_COUNT READ_ADDR WRITE_ADDR
 32 // Alias 3: CTRL WRITE_ADDR TRANS_COUNT READ_ADDR
 33 //
 34 // This will program the transfer count and read address of the data channel,
 35 // and trigger it. Once the data channel completes, it will restart the
 36 // control channel (via CHAIN_TO) to load the next two words into its control
 37 // registers.
 38
 39 const struct {uint32_t len; const char *data;} control_blocks[] = {
RP2350 Datasheet
12.6. DMA 1110
 40 {count_of(word0) - 1, word0}, // Skip null terminator
 41 {count_of(word1) - 1, word1},
 42 {count_of(word2) - 1, word2},
 43 {count_of(word3) - 1, word3},
 44 {count_of(word4) - 1, word4},
 45 {count_of(word5) - 1, word5},
 46 {0, NULL} // Null trigger to end chain.
 47 };
 48
 49 int main() {
 50 #ifndef uart_default
 51 #warning dma/control_blocks example requires a UART
 52 #else
 53 stdio_init_all();
 54 puts("DMA control block example:");
 55
 56 // ctrl_chan loads control blocks into data_chan, which executes them.
 57 int ctrl_chan = dma_claim_unused_channel(true);
 58 int data_chan = dma_claim_unused_channel(true);
 59
 60 // The control channel transfers two words into the data channel's control
 61 // registers, then halts. The write address wraps on a two-word
 62 // (eight-byte) boundary, so that the control channel writes the same two
 63 // registers when it is next triggered.
 64
 65 dma_channel_config c = dma_channel_get_default_config(ctrl_chan);
 66 channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
 67 channel_config_set_read_increment(&c, true);
 68 channel_config_set_write_increment(&c, true);
 69 channel_config_set_ring(&c, true, 3); // 1 << 3 byte boundary on write ptr
 70
 71 dma_channel_configure(
 72 ctrl_chan,
 73 &c,
 74 &dma_hw->ch[data_chan].al3_transfer_count, // Initial write address
 75 &control_blocks[0], // Initial read address
 76 2, // Halt after each control block
 77 false // Don't start yet
 78 );
 79
 80 // The data channel is set up to write to the UART FIFO (paced by the
 81 // UART's TX data request signal) and then chain to the control channel
 82 // once it completes. The control channel programs a new read address and
 83 // data length, and retriggers the data channel.
 84
 85 c = dma_channel_get_default_config(data_chan);
 86 channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
 87 channel_config_set_dreq(&c, uart_get_dreq(uart_default, true));
 88 // Trigger ctrl_chan when data_chan completes
 89 channel_config_set_chain_to(&c, ctrl_chan);
 90 // Raise the IRQ flag when 0 is written to a trigger register (end of chain):
 91 channel_config_set_irq_quiet(&c, true);
 92
 93 dma_channel_configure(
 94 data_chan,
 95 &c,
 96 &uart_get_hw(uart_default)->dr,
 97 NULL, // Initial read address and transfer count are unimportant;
 98 0, // the control channel will reprogram them each time.
 99 false // Don't start yet.
100 );
101
102 // Everything is ready to go. Tell the control channel to load the first
103 // control block. Everything is automatic from here.




register list



12.6.10. List of Registers
The DMA registers start at a base address of 0x50000000 (defined as DMA_BASE in SDK).
Table 1147. List of
DMA registers
Offset Name Info
0x000 CH0_READ_ADDR DMA Channel 0 Read Address pointer
0x004 CH0_WRITE_ADDR DMA Channel 0 Write Address pointer
0x008 CH0_TRANS_COUNT DMA Channel 0 Transfer Count
0x00c CH0_CTRL_TRIG DMA Channel 0 Control and Status
0x010 CH0_AL1_CTRL Alias for channel 0 CTRL register
0x014 CH0_AL1_READ_ADDR Alias for channel 0 READ_ADDR register
0x018 CH0_AL1_WRITE_ADDR Alias for channel 0 WRITE_ADDR register
0x01c CH0_AL1_TRANS_COUNT_TRIG Alias for channel 0 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x020 CH0_AL2_CTRL Alias for channel 0 CTRL register
0x024 CH0_AL2_TRANS_COUNT Alias for channel 0 TRANS_COUNT register
0x028 CH0_AL2_READ_ADDR Alias for channel 0 READ_ADDR register
0x02c CH0_AL2_WRITE_ADDR_TRIG Alias for channel 0 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x030 CH0_AL3_CTRL Alias for channel 0 CTRL register
0x034 CH0_AL3_WRITE_ADDR Alias for channel 0 WRITE_ADDR register
0x038 CH0_AL3_TRANS_COUNT Alias for channel 0 TRANS_COUNT register
0x03c CH0_AL3_READ_ADDR_TRIG Alias for channel 0 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x040 CH1_READ_ADDR DMA Channel 1 Read Address pointer
0x044 CH1_WRITE_ADDR DMA Channel 1 Write Address pointer
0x048 CH1_TRANS_COUNT DMA Channel 1 Transfer Count
0x04c CH1_CTRL_TRIG DMA Channel 1 Control and Status
RP2350 Datasheet
12.6. DMA 1112
Offset Name Info
0x050 CH1_AL1_CTRL Alias for channel 1 CTRL register
0x054 CH1_AL1_READ_ADDR Alias for channel 1 READ_ADDR register
0x058 CH1_AL1_WRITE_ADDR Alias for channel 1 WRITE_ADDR register
0x05c CH1_AL1_TRANS_COUNT_TRIG Alias for channel 1 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x060 CH1_AL2_CTRL Alias for channel 1 CTRL register
0x064 CH1_AL2_TRANS_COUNT Alias for channel 1 TRANS_COUNT register
0x068 CH1_AL2_READ_ADDR Alias for channel 1 READ_ADDR register
0x06c CH1_AL2_WRITE_ADDR_TRIG Alias for channel 1 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x070 CH1_AL3_CTRL Alias for channel 1 CTRL register
0x074 CH1_AL3_WRITE_ADDR Alias for channel 1 WRITE_ADDR register
0x078 CH1_AL3_TRANS_COUNT Alias for channel 1 TRANS_COUNT register
0x07c CH1_AL3_READ_ADDR_TRIG Alias for channel 1 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x080 CH2_READ_ADDR DMA Channel 2 Read Address pointer
0x084 CH2_WRITE_ADDR DMA Channel 2 Write Address pointer
0x088 CH2_TRANS_COUNT DMA Channel 2 Transfer Count
0x08c CH2_CTRL_TRIG DMA Channel 2 Control and Status
0x090 CH2_AL1_CTRL Alias for channel 2 CTRL register
0x094 CH2_AL1_READ_ADDR Alias for channel 2 READ_ADDR register
0x098 CH2_AL1_WRITE_ADDR Alias for channel 2 WRITE_ADDR register
0x09c CH2_AL1_TRANS_COUNT_TRIG Alias for channel 2 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x0a0 CH2_AL2_CTRL Alias for channel 2 CTRL register
0x0a4 CH2_AL2_TRANS_COUNT Alias for channel 2 TRANS_COUNT register
0x0a8 CH2_AL2_READ_ADDR Alias for channel 2 READ_ADDR register
0x0ac CH2_AL2_WRITE_ADDR_TRIG Alias for channel 2 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x0b0 CH2_AL3_CTRL Alias for channel 2 CTRL register
0x0b4 CH2_AL3_WRITE_ADDR Alias for channel 2 WRITE_ADDR register
0x0b8 CH2_AL3_TRANS_COUNT Alias for channel 2 TRANS_COUNT register
0x0bc CH2_AL3_READ_ADDR_TRIG Alias for channel 2 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
RP2350 Datasheet
12.6. DMA 1113
Offset Name Info
0x0c0 CH3_READ_ADDR DMA Channel 3 Read Address pointer
0x0c4 CH3_WRITE_ADDR DMA Channel 3 Write Address pointer
0x0c8 CH3_TRANS_COUNT DMA Channel 3 Transfer Count
0x0cc CH3_CTRL_TRIG DMA Channel 3 Control and Status
0x0d0 CH3_AL1_CTRL Alias for channel 3 CTRL register
0x0d4 CH3_AL1_READ_ADDR Alias for channel 3 READ_ADDR register
0x0d8 CH3_AL1_WRITE_ADDR Alias for channel 3 WRITE_ADDR register
0x0dc CH3_AL1_TRANS_COUNT_TRIG Alias for channel 3 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x0e0 CH3_AL2_CTRL Alias for channel 3 CTRL register
0x0e4 CH3_AL2_TRANS_COUNT Alias for channel 3 TRANS_COUNT register
0x0e8 CH3_AL2_READ_ADDR Alias for channel 3 READ_ADDR register
0x0ec CH3_AL2_WRITE_ADDR_TRIG Alias for channel 3 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x0f0 CH3_AL3_CTRL Alias for channel 3 CTRL register
0x0f4 CH3_AL3_WRITE_ADDR Alias for channel 3 WRITE_ADDR register
0x0f8 CH3_AL3_TRANS_COUNT Alias for channel 3 TRANS_COUNT register
0x0fc CH3_AL3_READ_ADDR_TRIG Alias for channel 3 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x100 CH4_READ_ADDR DMA Channel 4 Read Address pointer
0x104 CH4_WRITE_ADDR DMA Channel 4 Write Address pointer
0x108 CH4_TRANS_COUNT DMA Channel 4 Transfer Count
0x10c CH4_CTRL_TRIG DMA Channel 4 Control and Status
0x110 CH4_AL1_CTRL Alias for channel 4 CTRL register
0x114 CH4_AL1_READ_ADDR Alias for channel 4 READ_ADDR register
0x118 CH4_AL1_WRITE_ADDR Alias for channel 4 WRITE_ADDR register
0x11c CH4_AL1_TRANS_COUNT_TRIG Alias for channel 4 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x120 CH4_AL2_CTRL Alias for channel 4 CTRL register
0x124 CH4_AL2_TRANS_COUNT Alias for channel 4 TRANS_COUNT register
0x128 CH4_AL2_READ_ADDR Alias for channel 4 READ_ADDR register
0x12c CH4_AL2_WRITE_ADDR_TRIG Alias for channel 4 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x130 CH4_AL3_CTRL Alias for channel 4 CTRL register
RP2350 Datasheet
12.6. DMA 1114
Offset Name Info
0x134 CH4_AL3_WRITE_ADDR Alias for channel 4 WRITE_ADDR register
0x138 CH4_AL3_TRANS_COUNT Alias for channel 4 TRANS_COUNT register
0x13c CH4_AL3_READ_ADDR_TRIG Alias for channel 4 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x140 CH5_READ_ADDR DMA Channel 5 Read Address pointer
0x144 CH5_WRITE_ADDR DMA Channel 5 Write Address pointer
0x148 CH5_TRANS_COUNT DMA Channel 5 Transfer Count
0x14c CH5_CTRL_TRIG DMA Channel 5 Control and Status
0x150 CH5_AL1_CTRL Alias for channel 5 CTRL register
0x154 CH5_AL1_READ_ADDR Alias for channel 5 READ_ADDR register
0x158 CH5_AL1_WRITE_ADDR Alias for channel 5 WRITE_ADDR register
0x15c CH5_AL1_TRANS_COUNT_TRIG Alias for channel 5 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x160 CH5_AL2_CTRL Alias for channel 5 CTRL register
0x164 CH5_AL2_TRANS_COUNT Alias for channel 5 TRANS_COUNT register
0x168 CH5_AL2_READ_ADDR Alias for channel 5 READ_ADDR register
0x16c CH5_AL2_WRITE_ADDR_TRIG Alias for channel 5 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x170 CH5_AL3_CTRL Alias for channel 5 CTRL register
0x174 CH5_AL3_WRITE_ADDR Alias for channel 5 WRITE_ADDR register
0x178 CH5_AL3_TRANS_COUNT Alias for channel 5 TRANS_COUNT register
0x17c CH5_AL3_READ_ADDR_TRIG Alias for channel 5 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x180 CH6_READ_ADDR DMA Channel 6 Read Address pointer
0x184 CH6_WRITE_ADDR DMA Channel 6 Write Address pointer
0x188 CH6_TRANS_COUNT DMA Channel 6 Transfer Count
0x18c CH6_CTRL_TRIG DMA Channel 6 Control and Status
0x190 CH6_AL1_CTRL Alias for channel 6 CTRL register
0x194 CH6_AL1_READ_ADDR Alias for channel 6 READ_ADDR register
0x198 CH6_AL1_WRITE_ADDR Alias for channel 6 WRITE_ADDR register
0x19c CH6_AL1_TRANS_COUNT_TRIG Alias for channel 6 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x1a0 CH6_AL2_CTRL Alias for channel 6 CTRL register
0x1a4 CH6_AL2_TRANS_COUNT Alias for channel 6 TRANS_COUNT register
RP2350 Datasheet
12.6. DMA 1115
Offset Name Info
0x1a8 CH6_AL2_READ_ADDR Alias for channel 6 READ_ADDR register
0x1ac CH6_AL2_WRITE_ADDR_TRIG Alias for channel 6 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x1b0 CH6_AL3_CTRL Alias for channel 6 CTRL register
0x1b4 CH6_AL3_WRITE_ADDR Alias for channel 6 WRITE_ADDR register
0x1b8 CH6_AL3_TRANS_COUNT Alias for channel 6 TRANS_COUNT register
0x1bc CH6_AL3_READ_ADDR_TRIG Alias for channel 6 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x1c0 CH7_READ_ADDR DMA Channel 7 Read Address pointer
0x1c4 CH7_WRITE_ADDR DMA Channel 7 Write Address pointer
0x1c8 CH7_TRANS_COUNT DMA Channel 7 Transfer Count
0x1cc CH7_CTRL_TRIG DMA Channel 7 Control and Status
0x1d0 CH7_AL1_CTRL Alias for channel 7 CTRL register
0x1d4 CH7_AL1_READ_ADDR Alias for channel 7 READ_ADDR register
0x1d8 CH7_AL1_WRITE_ADDR Alias for channel 7 WRITE_ADDR register
0x1dc CH7_AL1_TRANS_COUNT_TRIG Alias for channel 7 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x1e0 CH7_AL2_CTRL Alias for channel 7 CTRL register
0x1e4 CH7_AL2_TRANS_COUNT Alias for channel 7 TRANS_COUNT register
0x1e8 CH7_AL2_READ_ADDR Alias for channel 7 READ_ADDR register
0x1ec CH7_AL2_WRITE_ADDR_TRIG Alias for channel 7 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x1f0 CH7_AL3_CTRL Alias for channel 7 CTRL register
0x1f4 CH7_AL3_WRITE_ADDR Alias for channel 7 WRITE_ADDR register
0x1f8 CH7_AL3_TRANS_COUNT Alias for channel 7 TRANS_COUNT register
0x1fc CH7_AL3_READ_ADDR_TRIG Alias for channel 7 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x200 CH8_READ_ADDR DMA Channel 8 Read Address pointer
0x204 CH8_WRITE_ADDR DMA Channel 8 Write Address pointer
0x208 CH8_TRANS_COUNT DMA Channel 8 Transfer Count
0x20c CH8_CTRL_TRIG DMA Channel 8 Control and Status
0x210 CH8_AL1_CTRL Alias for channel 8 CTRL register
0x214 CH8_AL1_READ_ADDR Alias for channel 8 READ_ADDR register
0x218 CH8_AL1_WRITE_ADDR Alias for channel 8 WRITE_ADDR register
RP2350 Datasheet
12.6. DMA 1116
Offset Name Info
0x21c CH8_AL1_TRANS_COUNT_TRIG Alias for channel 8 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x220 CH8_AL2_CTRL Alias for channel 8 CTRL register
0x224 CH8_AL2_TRANS_COUNT Alias for channel 8 TRANS_COUNT register
0x228 CH8_AL2_READ_ADDR Alias for channel 8 READ_ADDR register
0x22c CH8_AL2_WRITE_ADDR_TRIG Alias for channel 8 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x230 CH8_AL3_CTRL Alias for channel 8 CTRL register
0x234 CH8_AL3_WRITE_ADDR Alias for channel 8 WRITE_ADDR register
0x238 CH8_AL3_TRANS_COUNT Alias for channel 8 TRANS_COUNT register
0x23c CH8_AL3_READ_ADDR_TRIG Alias for channel 8 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x240 CH9_READ_ADDR DMA Channel 9 Read Address pointer
0x244 CH9_WRITE_ADDR DMA Channel 9 Write Address pointer
0x248 CH9_TRANS_COUNT DMA Channel 9 Transfer Count
0x24c CH9_CTRL_TRIG DMA Channel 9 Control and Status
0x250 CH9_AL1_CTRL Alias for channel 9 CTRL register
0x254 CH9_AL1_READ_ADDR Alias for channel 9 READ_ADDR register
0x258 CH9_AL1_WRITE_ADDR Alias for channel 9 WRITE_ADDR register
0x25c CH9_AL1_TRANS_COUNT_TRIG Alias for channel 9 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x260 CH9_AL2_CTRL Alias for channel 9 CTRL register
0x264 CH9_AL2_TRANS_COUNT Alias for channel 9 TRANS_COUNT register
0x268 CH9_AL2_READ_ADDR Alias for channel 9 READ_ADDR register
0x26c CH9_AL2_WRITE_ADDR_TRIG Alias for channel 9 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x270 CH9_AL3_CTRL Alias for channel 9 CTRL register
0x274 CH9_AL3_WRITE_ADDR Alias for channel 9 WRITE_ADDR register
0x278 CH9_AL3_TRANS_COUNT Alias for channel 9 TRANS_COUNT register
0x27c CH9_AL3_READ_ADDR_TRIG Alias for channel 9 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x280 CH10_READ_ADDR DMA Channel 10 Read Address pointer
0x284 CH10_WRITE_ADDR DMA Channel 10 Write Address pointer
0x288 CH10_TRANS_COUNT DMA Channel 10 Transfer Count
RP2350 Datasheet
12.6. DMA 1117
Offset Name Info
0x28c CH10_CTRL_TRIG DMA Channel 10 Control and Status
0x290 CH10_AL1_CTRL Alias for channel 10 CTRL register
0x294 CH10_AL1_READ_ADDR Alias for channel 10 READ_ADDR register
0x298 CH10_AL1_WRITE_ADDR Alias for channel 10 WRITE_ADDR register
0x29c CH10_AL1_TRANS_COUNT_TRIG Alias for channel 10 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x2a0 CH10_AL2_CTRL Alias for channel 10 CTRL register
0x2a4 CH10_AL2_TRANS_COUNT Alias for channel 10 TRANS_COUNT register
0x2a8 CH10_AL2_READ_ADDR Alias for channel 10 READ_ADDR register
0x2ac CH10_AL2_WRITE_ADDR_TRIG Alias for channel 10 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x2b0 CH10_AL3_CTRL Alias for channel 10 CTRL register
0x2b4 CH10_AL3_WRITE_ADDR Alias for channel 10 WRITE_ADDR register
0x2b8 CH10_AL3_TRANS_COUNT Alias for channel 10 TRANS_COUNT register
0x2bc CH10_AL3_READ_ADDR_TRIG Alias for channel 10 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x2c0 CH11_READ_ADDR DMA Channel 11 Read Address pointer
0x2c4 CH11_WRITE_ADDR DMA Channel 11 Write Address pointer
0x2c8 CH11_TRANS_COUNT DMA Channel 11 Transfer Count
0x2cc CH11_CTRL_TRIG DMA Channel 11 Control and Status
0x2d0 CH11_AL1_CTRL Alias for channel 11 CTRL register
0x2d4 CH11_AL1_READ_ADDR Alias for channel 11 READ_ADDR register
0x2d8 CH11_AL1_WRITE_ADDR Alias for channel 11 WRITE_ADDR register
0x2dc CH11_AL1_TRANS_COUNT_TRIG Alias for channel 11 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x2e0 CH11_AL2_CTRL Alias for channel 11 CTRL register
0x2e4 CH11_AL2_TRANS_COUNT Alias for channel 11 TRANS_COUNT register
0x2e8 CH11_AL2_READ_ADDR Alias for channel 11 READ_ADDR register
0x2ec CH11_AL2_WRITE_ADDR_TRIG Alias for channel 11 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x2f0 CH11_AL3_CTRL Alias for channel 11 CTRL register
0x2f4 CH11_AL3_WRITE_ADDR Alias for channel 11 WRITE_ADDR register
0x2f8 CH11_AL3_TRANS_COUNT Alias for channel 11 TRANS_COUNT register
RP2350 Datasheet
12.6. DMA 1118
Offset Name Info
0x2fc CH11_AL3_READ_ADDR_TRIG Alias for channel 11 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x300 CH12_READ_ADDR DMA Channel 12 Read Address pointer
0x304 CH12_WRITE_ADDR DMA Channel 12 Write Address pointer
0x308 CH12_TRANS_COUNT DMA Channel 12 Transfer Count
0x30c CH12_CTRL_TRIG DMA Channel 12 Control and Status
0x310 CH12_AL1_CTRL Alias for channel 12 CTRL register
0x314 CH12_AL1_READ_ADDR Alias for channel 12 READ_ADDR register
0x318 CH12_AL1_WRITE_ADDR Alias for channel 12 WRITE_ADDR register
0x31c CH12_AL1_TRANS_COUNT_TRIG Alias for channel 12 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x320 CH12_AL2_CTRL Alias for channel 12 CTRL register
0x324 CH12_AL2_TRANS_COUNT Alias for channel 12 TRANS_COUNT register
0x328 CH12_AL2_READ_ADDR Alias for channel 12 READ_ADDR register
0x32c CH12_AL2_WRITE_ADDR_TRIG Alias for channel 12 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x330 CH12_AL3_CTRL Alias for channel 12 CTRL register
0x334 CH12_AL3_WRITE_ADDR Alias for channel 12 WRITE_ADDR register
0x338 CH12_AL3_TRANS_COUNT Alias for channel 12 TRANS_COUNT register
0x33c CH12_AL3_READ_ADDR_TRIG Alias for channel 12 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x340 CH13_READ_ADDR DMA Channel 13 Read Address pointer
0x344 CH13_WRITE_ADDR DMA Channel 13 Write Address pointer
0x348 CH13_TRANS_COUNT DMA Channel 13 Transfer Count
0x34c CH13_CTRL_TRIG DMA Channel 13 Control and Status
0x350 CH13_AL1_CTRL Alias for channel 13 CTRL register
0x354 CH13_AL1_READ_ADDR Alias for channel 13 READ_ADDR register
0x358 CH13_AL1_WRITE_ADDR Alias for channel 13 WRITE_ADDR register
0x35c CH13_AL1_TRANS_COUNT_TRIG Alias for channel 13 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x360 CH13_AL2_CTRL Alias for channel 13 CTRL register
0x364 CH13_AL2_TRANS_COUNT Alias for channel 13 TRANS_COUNT register
0x368 CH13_AL2_READ_ADDR Alias for channel 13 READ_ADDR register
RP2350 Datasheet
12.6. DMA 1119
Offset Name Info
0x36c CH13_AL2_WRITE_ADDR_TRIG Alias for channel 13 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x370 CH13_AL3_CTRL Alias for channel 13 CTRL register
0x374 CH13_AL3_WRITE_ADDR Alias for channel 13 WRITE_ADDR register
0x378 CH13_AL3_TRANS_COUNT Alias for channel 13 TRANS_COUNT register
0x37c CH13_AL3_READ_ADDR_TRIG Alias for channel 13 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x380 CH14_READ_ADDR DMA Channel 14 Read Address pointer
0x384 CH14_WRITE_ADDR DMA Channel 14 Write Address pointer
0x388 CH14_TRANS_COUNT DMA Channel 14 Transfer Count
0x38c CH14_CTRL_TRIG DMA Channel 14 Control and Status
0x390 CH14_AL1_CTRL Alias for channel 14 CTRL register
0x394 CH14_AL1_READ_ADDR Alias for channel 14 READ_ADDR register
0x398 CH14_AL1_WRITE_ADDR Alias for channel 14 WRITE_ADDR register
0x39c CH14_AL1_TRANS_COUNT_TRIG Alias for channel 14 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x3a0 CH14_AL2_CTRL Alias for channel 14 CTRL register
0x3a4 CH14_AL2_TRANS_COUNT Alias for channel 14 TRANS_COUNT register
0x3a8 CH14_AL2_READ_ADDR Alias for channel 14 READ_ADDR register
0x3ac CH14_AL2_WRITE_ADDR_TRIG Alias for channel 14 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x3b0 CH14_AL3_CTRL Alias for channel 14 CTRL register
0x3b4 CH14_AL3_WRITE_ADDR Alias for channel 14 WRITE_ADDR register
0x3b8 CH14_AL3_TRANS_COUNT Alias for channel 14 TRANS_COUNT register
0x3bc CH14_AL3_READ_ADDR_TRIG Alias for channel 14 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x3c0 CH15_READ_ADDR DMA Channel 15 Read Address pointer
0x3c4 CH15_WRITE_ADDR DMA Channel 15 Write Address pointer
0x3c8 CH15_TRANS_COUNT DMA Channel 15 Transfer Count
0x3cc CH15_CTRL_TRIG DMA Channel 15 Control and Status
0x3d0 CH15_AL1_CTRL Alias for channel 15 CTRL register
0x3d4 CH15_AL1_READ_ADDR Alias for channel 15 READ_ADDR register
0x3d8 CH15_AL1_WRITE_ADDR Alias for channel 15 WRITE_ADDR register
RP2350 Datasheet
12.6. DMA 1120
Offset Name Info
0x3dc CH15_AL1_TRANS_COUNT_TRIG Alias for channel 15 TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x3e0 CH15_AL2_CTRL Alias for channel 15 CTRL register
0x3e4 CH15_AL2_TRANS_COUNT Alias for channel 15 TRANS_COUNT register
0x3e8 CH15_AL2_READ_ADDR Alias for channel 15 READ_ADDR register
0x3ec CH15_AL2_WRITE_ADDR_TRIG Alias for channel 15 WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x3f0 CH15_AL3_CTRL Alias for channel 15 CTRL register
0x3f4 CH15_AL3_WRITE_ADDR Alias for channel 15 WRITE_ADDR register
0x3f8 CH15_AL3_TRANS_COUNT Alias for channel 15 TRANS_COUNT register
0x3fc CH15_AL3_READ_ADDR_TRIG Alias for channel 15 READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
0x400 INTR Interrupt Status (raw)
0x404 INTE0 Interrupt Enables for IRQ 0
0x408 INTF0 Force Interrupts
0x40c INTS0 Interrupt Status for IRQ 0
0x414 INTE1 Interrupt Enables for IRQ 1
0x418 INTF1 Force Interrupts
0x41c INTS1 Interrupt Status for IRQ 1
0x424 INTE2 Interrupt Enables for IRQ 2
0x428 INTF2 Force Interrupts
0x42c INTS2 Interrupt Status for IRQ 2
0x434 INTE3 Interrupt Enables for IRQ 3
0x438 INTF3 Force Interrupts
0x43c INTS3 Interrupt Status for IRQ 3
0x440 TIMER0 Pacing timer (generate periodic TREQs)
0x444 TIMER1 Pacing timer (generate periodic TREQs)
0x448 TIMER2 Pacing timer (generate periodic TREQs)
0x44c TIMER3 Pacing timer (generate periodic TREQs)
0x450 MULTI_CHAN_TRIGGER Trigger one or more channels simultaneously
0x454 SNIFF_CTRL Sniffer Control
0x458 SNIFF_DATA Data accumulator for sniff hardware
0x460 FIFO_LEVELS Debug RAF, WAF, TDF levels
0x464 CHAN_ABORT Abort an in-progress transfer sequence on one or more channels
RP2350 Datasheet
12.6. DMA 1121
Offset Name Info
0x468 N_CHANNELS The number of channels this DMA instance is equipped with.
This DMA supports up to 16 hardware channels, but can be
configured with as few as one, to minimise silicon area.
0x480 SECCFG_CH0 Security level configuration for channel 0.
0x484 SECCFG_CH1 Security level configuration for channel 1.
0x488 SECCFG_CH2 Security level configuration for channel 2.
0x48c SECCFG_CH3 Security level configuration for channel 3.
0x490 SECCFG_CH4 Security level configuration for channel 4.
0x494 SECCFG_CH5 Security level configuration for channel 5.
0x498 SECCFG_CH6 Security level configuration for channel 6.
0x49c SECCFG_CH7 Security level configuration for channel 7.
0x4a0 SECCFG_CH8 Security level configuration for channel 8.
0x4a4 SECCFG_CH9 Security level configuration for channel 9.
0x4a8 SECCFG_CH10 Security level configuration for channel 10.
0x4ac SECCFG_CH11 Security level configuration for channel 11.
0x4b0 SECCFG_CH12 Security level configuration for channel 12.
0x4b4 SECCFG_CH13 Security level configuration for channel 13.
0x4b8 SECCFG_CH14 Security level configuration for channel 14.
0x4bc SECCFG_CH15 Security level configuration for channel 15.
0x4c0 SECCFG_IRQ0 Security configuration for IRQ 0. Control whether the IRQ permits
configuration by Non-secure/Unprivileged contexts, and whether
it can observe Secure/Privileged channel interrupt flags.
0x4c4 SECCFG_IRQ1 Security configuration for IRQ 1. Control whether the IRQ permits
configuration by Non-secure/Unprivileged contexts, and whether
it can observe Secure/Privileged channel interrupt flags.
0x4c8 SECCFG_IRQ2 Security configuration for IRQ 2. Control whether the IRQ permits
configuration by Non-secure/Unprivileged contexts, and whether
it can observe Secure/Privileged channel interrupt flags.
0x4cc SECCFG_IRQ3 Security configuration for IRQ 3. Control whether the IRQ permits
configuration by Non-secure/Unprivileged contexts, and whether
it can observe Secure/Privileged channel interrupt flags.
0x4d0 SECCFG_MISC Miscellaneous security configuration
0x500 MPU_CTRL Control register for DMA MPU. Accessible only from a Privileged
context.
0x504 MPU_BAR0 Base address register for MPU region 0. Writable only from a
Secure, Privileged context.
0x508 MPU_LAR0 Limit address register for MPU region 0. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x50c MPU_BAR1 Base address register for MPU region 1. Writable only from a
Secure, Privileged context.
RP2350 Datasheet
12.6. DMA 1122
Offset Name Info
0x510 MPU_LAR1 Limit address register for MPU region 1. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x514 MPU_BAR2 Base address register for MPU region 2. Writable only from a
Secure, Privileged context.
0x518 MPU_LAR2 Limit address register for MPU region 2. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x51c MPU_BAR3 Base address register for MPU region 3. Writable only from a
Secure, Privileged context.
0x520 MPU_LAR3 Limit address register for MPU region 3. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x524 MPU_BAR4 Base address register for MPU region 4. Writable only from a
Secure, Privileged context.
0x528 MPU_LAR4 Limit address register for MPU region 4. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x52c MPU_BAR5 Base address register for MPU region 5. Writable only from a
Secure, Privileged context.
0x530 MPU_LAR5 Limit address register for MPU region 5. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x534 MPU_BAR6 Base address register for MPU region 6. Writable only from a
Secure, Privileged context.
0x538 MPU_LAR6 Limit address register for MPU region 6. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x53c MPU_BAR7 Base address register for MPU region 7. Writable only from a
Secure, Privileged context.
0x540 MPU_LAR7 Limit address register for MPU region 7. Writable only from a
Secure, Privileged context, with the exception of the P bit.
0x800 CH0_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x804 CH0_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0x840 CH1_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x844 CH1_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0x880 CH2_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x884 CH2_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
RP2350 Datasheet
12.6. DMA 1123
Offset Name Info
0x8c0 CH3_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x8c4 CH3_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0x900 CH4_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x904 CH4_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0x940 CH5_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x944 CH5_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0x980 CH6_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x984 CH6_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0x9c0 CH7_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0x9c4 CH7_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xa00 CH8_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xa04 CH8_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xa40 CH9_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xa44 CH9_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xa80 CH10_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
RP2350 Datasheet
12.6. DMA 1124
Offset Name Info
0xa84 CH10_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xac0 CH11_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xac4 CH11_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xb00 CH12_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xb04 CH12_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xb40 CH13_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xb44 CH13_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xb80 CH14_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xb84 CH14_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
0xbc0 CH15_DBG_CTDREQ Read: get channel DREQ counter (i.e. how many accesses the
DMA expects it can perform on the peripheral without
overflow/underflow. Write any value: clears the counter, and
cause channel to re-initiate DREQ handshake.
0xbc4 CH15_DBG_TCR Read to get channel TRANS_COUNT reload value, i.e. the length
of the next transfer
DMA: CH0_READ_ADDR, CH1_READ_ADDR, …, CH14_READ_ADDR,
CH15_READ_ADDR Registers
Offsets: 0x000, 0x040, …, 0x380, 0x3c0
Description
DMA Channel N Read Address pointer
Table 1148.
CH0_READ_ADDR,
CH1_READ_ADDR, …,
CH14_READ_ADDR,
CH15_READ_ADDR
Registers
Bits Description Type Reset
31:0 This register updates automatically each time a read completes. The current
value is the next address to be read by this channel.
RW 0x00000000
DMA: CH0_WRITE_ADDR, CH1_WRITE_ADDR, …, CH14_WRITE_ADDR,
CH15_WRITE_ADDR Registers
Offsets: 0x004, 0x044, …, 0x384, 0x3c4
RP2350 Datasheet
12.6. DMA 1125
Description
DMA Channel N Write Address pointer
Table 1149.
CH0_WRITE_ADDR,
CH1_WRITE_ADDR, …,
CH14_WRITE_ADDR,
CH15_WRITE_ADDR
Registers
Bits Description Type Reset
31:0 This register updates automatically each time a write completes. The current
value is the next address to be written by this channel.
RW 0x00000000
DMA: CH0_TRANS_COUNT, CH1_TRANS_COUNT, …, CH14_TRANS_COUNT,
CH15_TRANS_COUNT Registers
Offsets: 0x008, 0x048, …, 0x388, 0x3c8
Description
DMA Channel N Transfer Count
Table 1150.
CH0_TRANS_COUNT,
CH1_TRANS_COUNT,
…,
CH14_TRANS_COUNT,
CH15_TRANS_COUNT
Registers
Bits Description Type Reset
31:28 MODE: When MODE is 0x0, the transfer count decrements with each transfer
until 0, and then the channel triggers the next channel indicated by
CTRL_CHAIN_TO.
When MODE is 0x1, the transfer count decrements with each transfer until 0,
and then the channel re-triggers itself, in addition to the trigger indicated by
CTRL_CHAIN_TO. This is useful for e.g. an endless ring-buffer DMA with
periodic interrupts.
When MODE is 0xf, the transfer count does not decrement. The DMA channel
performs an endless sequence of transfers, never triggering other channels or
raising interrupts, until an ABORT is raised.
All other values are reserved.
RW 0x0
Enumerated values:
0x0 → NORMAL
0x1 → TRIGGER_SELF
0xf → ENDLESS
27:0 COUNT: 28-bit transfer count (256 million transfers maximum).
Program the number of bus transfers a channel will perform before halting.
Note that, if transfers are larger than one byte in size, this is not equal to the
number of bytes transferred (see CTRL_DATA_SIZE).
When the channel is active, reading this register shows the number of
transfers remaining, updating automatically each time a write transfer
completes.
Writing this register sets the RELOAD value for the transfer counter. Each time
this channel is triggered, the RELOAD value is copied into the live transfer
counter. The channel can be started multiple times, and will perform the same
number of transfers each time, as programmed by most recent write.
The RELOAD value can be observed at CHx_DBG_TCR. If TRANS_COUNT is
used as a trigger, the written value is used immediately as the length of the
new transfer sequence, as well as being written to RELOAD.
RW 0x0000000
RP2350 Datasheet
12.6. DMA 1126
DMA: CH0_CTRL_TRIG, CH1_CTRL_TRIG, …, CH14_CTRL_TRIG,
CH15_CTRL_TRIG Registers
Offsets: 0x00c, 0x04c, …, 0x38c, 0x3cc
Description
DMA Channel N Control and Status
Table 1151.
CH0_CTRL_TRIG,
CH1_CTRL_TRIG, …,
CH14_CTRL_TRIG,
CH15_CTRL_TRIG
Registers
Bits Description Type Reset
31 AHB_ERROR: Logical OR of the READ_ERROR and WRITE_ERROR flags. The
channel halts when it encounters any bus error, and always raises its channel
IRQ flag.
RO 0x0
30 READ_ERROR: If 1, the channel received a read bus error. Write one to clear.
READ_ADDR shows the approximate address where the bus error was
encountered (will not be earlier, or more than 3 transfers later)
WC 0x0
29 WRITE_ERROR: If 1, the channel received a write bus error. Write one to clear.
WRITE_ADDR shows the approximate address where the bus error was
encountered (will not be earlier, or more than 5 transfers later)
WC 0x0
28:27 Reserved. - -
26 BUSY: This flag goes high when the channel starts a new transfer sequence,
and low when the last transfer of that sequence completes. Clearing EN while
BUSY is high pauses the channel, and BUSY will stay high while paused.
To terminate a sequence early (and clear the BUSY flag), see CHAN_ABORT.
RO 0x0
25 SNIFF_EN: If 1, this channel’s data transfers are visible to the sniff hardware,
and each transfer will advance the state of the checksum. This only applies if
the sniff hardware is enabled, and has this channel selected.
This allows checksum to be enabled or disabled on a per-control- block basis.
RW 0x0
24 BSWAP: Apply byte-swap transformation to DMA data.
For byte data, this has no effect. For halfword data, the two bytes of each
halfword are swapped. For word data, the four bytes of each word are
swapped to reverse order.
RW 0x0
23 IRQ_QUIET: In QUIET mode, the channel does not generate IRQs at the end of
every transfer block. Instead, an IRQ is raised when NULL is written to a trigger
register, indicating the end of a control block chain.
This reduces the number of interrupts to be serviced by the CPU when
transferring a DMA chain of many small control blocks.
RW 0x0
22:17 TREQ_SEL: Select a Transfer Request signal.
The channel uses the transfer request signal to pace its data transfer rate.
Sources for TREQ signals are internal (TIMERS) or external (DREQ, a Data
Request from the system).
0x0 to 0x3a → select DREQ n as TREQ
RW 0x00
Enumerated values:
0x3b → TIMER0: Select Timer 0 as TREQ
0x3c → TIMER1: Select Timer 1 as TREQ
0x3d → TIMER2: Select Timer 2 as TREQ (Optional)
0x3e → TIMER3: Select Timer 3 as TREQ (Optional)
RP2350 Datasheet
12.6. DMA 1127
Bits Description Type Reset
0x3f → PERMANENT: Permanent request, for unpaced transfers.
16:13 CHAIN_TO: When this channel completes, it will trigger the channel indicated
by CHAIN_TO. Disable by setting CHAIN_TO = (this channel).
Note this field resets to 0, so channels 1 and above will chain to channel 0 by
default. Set this field to avoid this behaviour.
RW 0x0
12 RING_SEL: Select whether RING_SIZE applies to read or write addresses.
If 0, read addresses are wrapped on a (1 << RING_SIZE) boundary. If 1, write
addresses are wrapped.
RW 0x0
11:8 RING_SIZE: Size of address wrap region. If 0, don’t wrap. For values n > 0, only
the lower n bits of the address will change. This wraps the address on a (1 <<
n) byte boundary, facilitating access to naturally-aligned ring buffers.
Ring sizes between 2 and 32768 bytes are possible. This can apply to either
read or write addresses, based on value of RING_SEL.
RW 0x0
Enumerated values:
0x0 → RING_NONE
7 INCR_WRITE_REV: If 1, and INCR_WRITE is 1, the write address is
decremented rather than incremented with each transfer.
If 1, and INCR_WRITE is 0, this otherwise-unused combination causes the
write address to be incremented by twice the transfer size, i.e. skipping over
alternate addresses.
RW 0x0
6 INCR_WRITE: If 1, the write address increments with each transfer. If 0, each
write is directed to the same, initial address.
Generally this should be disabled for memory-to-peripheral transfers.
RW 0x0
5 INCR_READ_REV: If 1, and INCR_READ is 1, the read address is decremented
rather than incremented with each transfer.
If 1, and INCR_READ is 0, this otherwise-unused combination causes the read
address to be incremented by twice the transfer size, i.e. skipping over
alternate addresses.
RW 0x0
4 INCR_READ: If 1, the read address increments with each transfer. If 0, each
read is directed to the same, initial address.
Generally this should be disabled for peripheral-to-memory transfers.
RW 0x0
3:2 DATA_SIZE: Set the size of each bus transfer (byte/halfword/word).
READ_ADDR and WRITE_ADDR advance by this amount (1/2/4 bytes) with
each transfer.
RW 0x0
Enumerated values:
0x0 → SIZE_BYTE
0x1 → SIZE_HALFWORD
0x2 → SIZE_WORD
RP2350 Datasheet
12.6. DMA 1128
Bits Description Type Reset
1 HIGH_PRIORITY: HIGH_PRIORITY gives a channel preferential treatment in
issue scheduling: in each scheduling round, all high priority channels are
considered first, and then only a single low priority channel, before returning to
the high priority channels.
This only affects the order in which the DMA schedules channels. The DMA’s
bus priority is not changed. If the DMA is not saturated then a low priority
channel will see no loss of throughput.
RW 0x0
0 EN: DMA Channel Enable.
When 1, the channel will respond to triggering events, which will cause it to
become BUSY and start transferring data. When 0, the channel will ignore
triggers, stop issuing transfers, and pause the current transfer sequence (i.e.
BUSY will remain high if already high)
RW 0x0
DMA: CH0_AL1_CTRL, CH1_AL1_CTRL, …, CH14_AL1_CTRL, CH15_AL1_CTRL
Registers
Offsets: 0x010, 0x050, …, 0x390, 0x3d0
Table 1152.
CH0_AL1_CTRL,
CH1_AL1_CTRL, …,
CH14_AL1_CTRL,
CH15_AL1_CTRL
Registers
Bits Description Type Reset
31:0 Alias for channel N CTRL register RW -
DMA: CH0_AL1_READ_ADDR, CH1_AL1_READ_ADDR, …,
CH14_AL1_READ_ADDR, CH15_AL1_READ_ADDR Registers
Offsets: 0x014, 0x054, …, 0x394, 0x3d4
Table 1153.
CH0_AL1_READ_ADDR
,
CH1_AL1_READ_ADDR
, …,
CH14_AL1_READ_ADD
R,
CH15_AL1_READ_ADD
R Registers
Bits Description Type Reset
31:0 Alias for channel N READ_ADDR register RW -
DMA: CH0_AL1_WRITE_ADDR, CH1_AL1_WRITE_ADDR, …,
CH14_AL1_WRITE_ADDR, CH15_AL1_WRITE_ADDR Registers
Offsets: 0x018, 0x058, …, 0x398, 0x3d8
Table 1154.
CH0_AL1_WRITE_ADD
R,
CH1_AL1_WRITE_ADD
R, …,
CH14_AL1_WRITE_AD
DR,
CH15_AL1_WRITE_AD
DR Registers
Bits Description Type Reset
31:0 Alias for channel N WRITE_ADDR register RW -
DMA: CH0_AL1_TRANS_COUNT_TRIG, CH1_AL1_TRANS_COUNT_TRIG, …,
CH14_AL1_TRANS_COUNT_TRIG, CH15_AL1_TRANS_COUNT_TRIG Registers
Offsets: 0x01c, 0x05c, …, 0x39c, 0x3dc
Table 1155.
CH0_AL1_TRANS_COU
NT_TRIG,
CH1_AL1_TRANS_COU
NT_TRIG, …,
CH14_AL1_TRANS_CO
UNT_TRIG,
CH15_AL1_TRANS_CO
UNT_TRIG Registers
Bits Description Type Reset
31:0 Alias for channel N TRANS_COUNT register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
RW -
DMA: CH0_AL2_CTRL, CH1_AL2_CTRL, …, CH14_AL2_CTRL, CH15_AL2_CTRL
Registers
RP2350 Datasheet
12.6. DMA 1129
Offsets: 0x020, 0x060, …, 0x3a0, 0x3e0
Table 1156.
CH0_AL2_CTRL,
CH1_AL2_CTRL, …,
CH14_AL2_CTRL,
CH15_AL2_CTRL
Registers
Bits Description Type Reset
31:0 Alias for channel N CTRL register RW -
DMA: CH0_AL2_TRANS_COUNT, CH1_AL2_TRANS_COUNT, …,
CH14_AL2_TRANS_COUNT, CH15_AL2_TRANS_COUNT Registers
Offsets: 0x024, 0x064, …, 0x3a4, 0x3e4
Table 1157.
CH0_AL2_TRANS_COU
NT,
CH1_AL2_TRANS_COU
NT, …,
CH14_AL2_TRANS_CO
UNT,
CH15_AL2_TRANS_CO
UNT Registers
Bits Description Type Reset
31:0 Alias for channel N TRANS_COUNT register RW -
DMA: CH0_AL2_READ_ADDR, CH1_AL2_READ_ADDR, …,
CH14_AL2_READ_ADDR, CH15_AL2_READ_ADDR Registers
Offsets: 0x028, 0x068, …, 0x3a8, 0x3e8
Table 1158.
CH0_AL2_READ_ADDR
,
CH1_AL2_READ_ADDR
, …,
CH14_AL2_READ_ADD
R,
CH15_AL2_READ_ADD
R Registers
Bits Description Type Reset
31:0 Alias for channel N READ_ADDR register RW -
DMA: CH0_AL2_WRITE_ADDR_TRIG, CH1_AL2_WRITE_ADDR_TRIG, …,
CH14_AL2_WRITE_ADDR_TRIG, CH15_AL2_WRITE_ADDR_TRIG Registers
Offsets: 0x02c, 0x06c, …, 0x3ac, 0x3ec
Table 1159.
CH0_AL2_WRITE_ADD
R_TRIG,
CH1_AL2_WRITE_ADD
R_TRIG, …,
CH14_AL2_WRITE_AD
DR_TRIG,
CH15_AL2_WRITE_AD
DR_TRIG Registers
Bits Description Type Reset
31:0 Alias for channel N WRITE_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
RW -
DMA: CH0_AL3_CTRL, CH1_AL3_CTRL, …, CH14_AL3_CTRL, CH15_AL3_CTRL
Registers
Offsets: 0x030, 0x070, …, 0x3b0, 0x3f0
Table 1160.
CH0_AL3_CTRL,
CH1_AL3_CTRL, …,
CH14_AL3_CTRL,
CH15_AL3_CTRL
Registers
Bits Description Type Reset
31:0 Alias for channel N CTRL register RW -
DMA: CH0_AL3_WRITE_ADDR, CH1_AL3_WRITE_ADDR, …,
CH14_AL3_WRITE_ADDR, CH15_AL3_WRITE_ADDR Registers
Offsets: 0x034, 0x074, …, 0x3b4, 0x3f4
Table 1161.
CH0_AL3_WRITE_ADD
R,
CH1_AL3_WRITE_ADD
R, …,
CH14_AL3_WRITE_AD
DR,
CH15_AL3_WRITE_AD
DR Registers
Bits Description Type Reset
31:0 Alias for channel N WRITE_ADDR register RW -
DMA: CH0_AL3_TRANS_COUNT, CH1_AL3_TRANS_COUNT, …,
CH14_AL3_TRANS_COUNT, CH15_AL3_TRANS_COUNT Registers
Offsets: 0x038, 0x078, …, 0x3b8, 0x3f8
RP2350 Datasheet
12.6. DMA 1130
Table 1162.
CH0_AL3_TRANS_COU
NT,
CH1_AL3_TRANS_COU
NT, …,
CH14_AL3_TRANS_CO
UNT,
CH15_AL3_TRANS_CO
UNT Registers
Bits Description Type Reset
31:0 Alias for channel N TRANS_COUNT register RW -
DMA: CH0_AL3_READ_ADDR_TRIG, CH1_AL3_READ_ADDR_TRIG, …,
CH14_AL3_READ_ADDR_TRIG, CH15_AL3_READ_ADDR_TRIG Registers
Offsets: 0x03c, 0x07c, …, 0x3bc, 0x3fc
Table 1163.
CH0_AL3_READ_ADDR
_TRIG,
CH1_AL3_READ_ADDR
_TRIG, …,
CH14_AL3_READ_ADD
R_TRIG,
CH15_AL3_READ_ADD
R_TRIG Registers
Bits Description Type Reset
31:0 Alias for channel N READ_ADDR register
This is a trigger register (0xc). Writing a nonzero value will
reload the channel counter and start the channel.
RW -
DMA: INTR Register
Offset: 0x400
Description
Interrupt Status (raw)
Table 1164. INTR
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Raw interrupt status for DMA Channels 0..15. Bit n corresponds to channel n.
Ignores any masking or forcing. Channel interrupts can be cleared by writing a
bit mask to INTR or INTS0/1/2/3.
Channel interrupts can be routed to either of four system-level IRQs based on
INTE0, INTE1, INTE2 and INTE3.
The multiple system-level interrupts might be used to allow NVIC IRQ
preemption for more time-critical channels, to spread IRQ load across
different cores, or to target IRQs to different security domains.
It is also valid to ignore the multiple IRQs, and just use INTE0/INTS0/IRQ 0.
If this register is accessed at a security/privilege level less than that of a given
channel (as defined by that channel’s SECCFG_CHx register), then that
channel’s interrupt status will read as 0, ignore writes.
WC 0x0000
DMA: INTE0 Register
Offset: 0x404
Description
Interrupt Enables for IRQ 0
RP2350 Datasheet
12.6. DMA 1131
Table 1165. INTE0
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Set bit n to pass interrupts from channel n to DMA IRQ 0.
Note this bit has no effect if the channel security/privilege level, defined by
SECCFG_CHx, is greater than the IRQ security/privilege defined by
SECCFG_IRQ0.
RW 0x0000
DMA: INTF0 Register
Offset: 0x408
Description
Force Interrupts
Table 1166. INTF0
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Write 1s to force the corresponding bits in INTS0. The interrupt remains
asserted until INTF0 is cleared.
RW 0x0000
DMA: INTS0 Register
Offset: 0x40c
Description
Interrupt Status for IRQ 0
Table 1167. INTS0
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Indicates active channel interrupt requests which are currently causing IRQ 0
to be asserted.
Channel interrupts can be cleared by writing a bit mask here.
Channels with a security/privilege (SECCFG_CHx) greater SECCFG_IRQ0) read
as 0 in this register, and ignore writes.
WC 0x0000
DMA: INTE1 Register
Offset: 0x414
Description
Interrupt Enables for IRQ 1
RP2350 Datasheet
12.6. DMA 1132
Table 1168. INTE1
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Set bit n to pass interrupts from channel n to DMA IRQ 1.
Note this bit has no effect if the channel security/privilege level, defined by
SECCFG_CHx, is greater than the IRQ security/privilege defined by
SECCFG_IRQ1.
RW 0x0000
DMA: INTF1 Register
Offset: 0x418
Description
Force Interrupts
Table 1169. INTF1
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Write 1s to force the corresponding bits in INTS1. The interrupt remains
asserted until INTF1 is cleared.
RW 0x0000
DMA: INTS1 Register
Offset: 0x41c
Description
Interrupt Status for IRQ 1
Table 1170. INTS1
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Indicates active channel interrupt requests which are currently causing IRQ 1
to be asserted.
Channel interrupts can be cleared by writing a bit mask here.
Channels with a security/privilege (SECCFG_CHx) greater SECCFG_IRQ1) read
as 0 in this register, and ignore writes.
WC 0x0000
DMA: INTE2 Register
Offset: 0x424
Description
Interrupt Enables for IRQ 2
RP2350 Datasheet
12.6. DMA 1133
Table 1171. INTE2
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Set bit n to pass interrupts from channel n to DMA IRQ 2.
Note this bit has no effect if the channel security/privilege level, defined by
SECCFG_CHx, is greater than the IRQ security/privilege defined by
SECCFG_IRQ2.
RW 0x0000
DMA: INTF2 Register
Offset: 0x428
Description
Force Interrupts
Table 1172. INTF2
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Write 1s to force the corresponding bits in INTS2. The interrupt remains
asserted until INTF2 is cleared.
RW 0x0000
DMA: INTS2 Register
Offset: 0x42c
Description
Interrupt Status for IRQ 2
Table 1173. INTS2
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Indicates active channel interrupt requests which are currently causing IRQ 2
to be asserted.
Channel interrupts can be cleared by writing a bit mask here.
Channels with a security/privilege (SECCFG_CHx) greater SECCFG_IRQ2) read
as 0 in this register, and ignore writes.
WC 0x0000
DMA: INTE3 Register
Offset: 0x434
Description
Interrupt Enables for IRQ 3
RP2350 Datasheet
12.6. DMA 1134
Table 1174. INTE3
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Set bit n to pass interrupts from channel n to DMA IRQ 3.
Note this bit has no effect if the channel security/privilege level, defined by
SECCFG_CHx, is greater than the IRQ security/privilege defined by
SECCFG_IRQ3.
RW 0x0000
DMA: INTF3 Register
Offset: 0x438
Description
Force Interrupts
Table 1175. INTF3
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Write 1s to force the corresponding bits in INTS3. The interrupt remains
asserted until INTF3 is cleared.
RW 0x0000
DMA: INTS3 Register
Offset: 0x43c
Description
Interrupt Status for IRQ 3
Table 1176. INTS3
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Indicates active channel interrupt requests which are currently causing IRQ 3
to be asserted.
Channel interrupts can be cleared by writing a bit mask here.
Channels with a security/privilege (SECCFG_CHx) greater SECCFG_IRQ3) read
as 0 in this register, and ignore writes.
WC 0x0000
DMA: TIMER0, TIMER1, TIMER2, TIMER3 Registers
Offsets: 0x440, 0x444, 0x448, 0x44c
Description
Pacing (X/Y) fractional timer
The pacing timer produces TREQ assertions at a rate set by ((X/Y) * sys_clk). This equation is evaluated every
sys_clk cycles and therefore can only generate TREQs at a rate of 1 per sys_clk (i.e. permanent TREQ) or less.
Table 1177. TIMER0,
TIMER1, TIMER2,
TIMER3 Registers
Bits Description Type Reset
31:16 X: Pacing Timer Dividend. Specifies the X value for the (X/Y) fractional timer. RW 0x0000
15:0 Y: Pacing Timer Divisor. Specifies the Y value for the (X/Y) fractional timer. RW 0x0000
DMA: MULTI_CHAN_TRIGGER Register
Offset: 0x450
RP2350 Datasheet
12.6. DMA 1135
Description
Trigger one or more channels simultaneously
Table 1178.
MULTI_CHAN_TRIGGE
R Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Each bit in this register corresponds to a DMA channel. Writing a 1 to the
relevant bit is the same as writing to that channel’s trigger register; the
channel will start if it is currently enabled and not already busy.
SC 0x0000
DMA: SNIFF_CTRL Register
Offset: 0x454
Description
Sniffer Control
Table 1179.
SNIFF_CTRL Register
Bits Description Type Reset
31:12 Reserved. - -
11 OUT_INV: If set, the result appears inverted (bitwise complement) when read.
This does not affect the way the checksum is calculated; the result is
transformed on-the-fly between the result register and the bus.
RW 0x0
10 OUT_REV: If set, the result appears bit-reversed when read. This does not
affect the way the checksum is calculated; the result is transformed on-the-fly
between the result register and the bus.
RW 0x0
9 BSWAP: Locally perform a byte reverse on the sniffed data, before feeding into
checksum.
Note that the sniff hardware is downstream of the DMA channel byteswap
performed in the read master: if channel CTRL_BSWAP and
SNIFF_CTRL_BSWAP are both enabled, their effects cancel from the sniffer’s
point of view.
RW 0x0
8:5 CALC RW 0x0
Enumerated values:
0x0 → CRC32: Calculate a CRC-32 (IEEE802.3 polynomial)
0x1 → CRC32R: Calculate a CRC-32 (IEEE802.3 polynomial) with bit reversed
data
0x2 → CRC16: Calculate a CRC-16-CCITT
0x3 → CRC16R: Calculate a CRC-16-CCITT with bit reversed data
0xe → EVEN: XOR reduction over all data. == 1 if the total 1 population count
is odd.
0xf → SUM: Calculate a simple 32-bit checksum (addition with a 32 bit
accumulator)
4:1 DMACH: DMA channel for Sniffer to observe RW 0x0
0 EN: Enable sniffer RW 0x0
DMA: SNIFF_DATA Register
Offset: 0x458
RP2350 Datasheet
12.6. DMA 1136
Description
Data accumulator for sniff hardware
Table 1180.
SNIFF_DATA Register
Bits Description Type Reset
31:0 Write an initial seed value here before starting a DMA transfer on the channel
indicated by SNIFF_CTRL_DMACH. The hardware will update this register each
time it observes a read from the indicated channel. Once the channel
completes, the final result can be read from this register.
RW 0x00000000
DMA: FIFO_LEVELS Register
Offset: 0x460
Description
Debug RAF, WAF, TDF levels
Table 1181.
FIFO_LEVELS Register
Bits Description Type Reset
31:24 Reserved. - -
23:16 RAF_LVL: Current Read-Address-FIFO fill level RO 0x00
15:8 WAF_LVL: Current Write-Address-FIFO fill level RO 0x00
7:0 TDF_LVL: Current Transfer-Data-FIFO fill level RO 0x00
DMA: CHAN_ABORT Register
Offset: 0x464
Description
Abort an in-progress transfer sequence on one or more channels
Table 1182.
CHAN_ABORT
Register
Bits Description Type Reset
31:16 Reserved. - -
15:0 Each bit corresponds to a channel. Writing a 1 aborts whatever transfer
sequence is in progress on that channel. The bit will remain high until any inflight transfers have been flushed through the address and data FIFOs.
After writing, this register must be polled until it returns all-zero. Until this
point, it is unsafe to restart the channel.
SC 0x0000
DMA: N_CHANNELS Register
Offset: 0x468
Table 1183.
N_CHANNELS Register
Bits Description Type Reset
31:5 Reserved. - -
4:0 The number of channels this DMA instance is equipped with. This DMA
supports up to 16 hardware channels, but can be configured with as few as
one, to minimise silicon area.
RO -
DMA: SECCFG_CH0, SECCFG_CH1, …, SECCFG_CH14, SECCFG_CH15
Registers
Offsets: 0x480, 0x484, …, 0x4b8, 0x4bc
RP2350 Datasheet
12.6. DMA 1137
Description
Security configuration for channel N. Control whether this channel performs Secure/Non-secure and
Privileged/Unprivileged bus accesses.
If this channel generates bus accesses of some security level, an access of at least that level (in the order S+P > S+U >
NS+P > NS+U) is required to program, trigger, abort, check the status of, interrupt on or acknowledge the interrupt of
this channel.
This register automatically locks down (becomes read-only) once software starts to configure the channel.
This register is world-readable, but is writable only from a Secure, Privileged context.
Table 1184.
SECCFG_CH0,
SECCFG_CH1, …,
SECCFG_CH14,
SECCFG_CH15
Registers
Bits Description Type Reset
31:3 Reserved. - -
2 LOCK: LOCK is 0 at reset, and is set to 1 automatically upon a successful write
to this channel’s control registers. That is, a write to CTRL, READ_ADDR,
WRITE_ADDR, TRANS_COUNT and their aliases.
Once its LOCK bit is set, this register becomes read-only.
A failed write, for example due to the write’s privilege being lower than that
specified in the channel’s SECCFG register, will not set the LOCK bit.
RW 0x0
1 S: Secure channel. If 1, this channel performs Secure bus accesses. If 0, it
performs Non-secure bus accesses.
If 1, this channel is controllable only from a Secure context.
RW 0x1
0 P: Privileged channel. If 1, this channel performs Privileged bus accesses. If 0,
it performs Unprivileged bus accesses.
If 1, this channel is controllable only from a Privileged context of the same
Secure/Non-secure level, or any context of a higher Secure/Non-secure level.
RW 0x1
DMA: SECCFG_IRQ0, SECCFG_IRQ1, SECCFG_IRQ2, SECCFG_IRQ3 Registers
Offsets: 0x4c0, 0x4c4, 0x4c8, 0x4cc
Description
Security configuration for IRQ N. Control whether the IRQ permits configuration by Non-secure/Unprivileged
contexts, and whether it can observe Secure/Privileged channel interrupt flags.
Table 1185.
SECCFG_IRQ0,
SECCFG_IRQ1,
SECCFG_IRQ2,
SECCFG_IRQ3
Registers
Bits Description Type Reset
31:2 Reserved. - -
1 S: Secure IRQ. If 1, this IRQ’s control registers can only be accessed from a
Secure context.
If 0, this IRQ’s control registers can be accessed from a Non-secure context,
but Secure channels (as per SECCFG_CHx) are masked from the IRQ status,
and this IRQ’s registers can not be used to acknowledge the channel interrupts
of Secure channels.
RW 0x1
RP2350 Datasheet
12.6. DMA 1138
Bits Description Type Reset
0 P: Privileged IRQ. If 1, this IRQ’s control registers can only be accessed from a
Privileged context.
If 0, this IRQ’s control registers can be accessed from an Unprivileged context,
but Privileged channels (as per SECCFG_CHx) are masked from the IRQ status,
and this IRQ’s registers can not be used to acknowledge the channel interrupts
of Privileged channels.
RW 0x1
DMA: SECCFG_MISC Register
Offset: 0x4d0
Description
Miscellaneous security configuration
Table 1186.
SECCFG_MISC
Register
Bits Description Type Reset
31:10 Reserved. - -
9 TIMER3_S: If 1, the TIMER3 register is only accessible from a Secure context,
and timer DREQ 3 is only visible to Secure channels.
RW 0x1
8 TIMER3_P: If 1, the TIMER3 register is only accessible from a Privileged (or
more Secure) context, and timer DREQ 3 is only visible to Privileged (or more
Secure) channels.
RW 0x1
7 TIMER2_S: If 1, the TIMER2 register is only accessible from a Secure context,
and timer DREQ 2 is only visible to Secure channels.
RW 0x1
6 TIMER2_P: If 1, the TIMER2 register is only accessible from a Privileged (or
more Secure) context, and timer DREQ 2 is only visible to Privileged (or more
Secure) channels.
RW 0x1
5 TIMER1_S: If 1, the TIMER1 register is only accessible from a Secure context,
and timer DREQ 1 is only visible to Secure channels.
RW 0x1
4 TIMER1_P: If 1, the TIMER1 register is only accessible from a Privileged (or
more Secure) context, and timer DREQ 1 is only visible to Privileged (or more
Secure) channels.
RW 0x1
3 TIMER0_S: If 1, the TIMER0 register is only accessible from a Secure context,
and timer DREQ 0 is only visible to Secure channels.
RW 0x1
2 TIMER0_P: If 1, the TIMER0 register is only accessible from a Privileged (or
more Secure) context, and timer DREQ 0 is only visible to Privileged (or more
Secure) channels.
RW 0x1
1 SNIFF_S: If 1, the sniffer can see data transfers from Secure channels, and can
itself only be accessed from a Secure context.
If 0, the sniffer can be accessed from either a Secure or Non-secure context,
but can not see data transfers of Secure channels.
RW 0x1
0 SNIFF_P: If 1, the sniffer can see data transfers from Privileged channels, and
can itself only be accessed from a privileged context, or from a Secure context
when SNIFF_S is 0.
If 0, the sniffer can be accessed from either a Privileged or Unprivileged
context (with sufficient security level) but can not see transfers from
Privileged channels.
RW 0x1
RP2350 Datasheet
12.6. DMA 1139
DMA: MPU_CTRL Register
Offset: 0x500
Description
Control register for DMA MPU. Accessible only from a Privileged context.
Table 1187.
MPU_CTRL Register
Bits Description Type Reset
31:4 Reserved. - -
3 NS_HIDE_ADDR: By default, when a region’s S bit is clear, Non-securePrivileged reads can see the region’s base address and limit address. Set this
bit to make the addresses appear as 0 to Non-secure reads, even when the
region is Non-secure, to avoid leaking information about the processor SAU
map.
RW 0x0
2 S: Determine whether an address not covered by an active MPU region is
Secure (1) or Non-secure (0)
RW 0x0
1 P: Determine whether an address not covered by an active MPU region is
Privileged (1) or Unprivileged (0)
RW 0x0
0 Reserved. - -
DMA: MPU_BAR0, MPU_BAR1, …, MPU_BAR6, MPU_BAR7 Registers
Offsets: 0x504, 0x50c, …, 0x534, 0x53c
Description
Base address register for MPU region N. Writable only from a Secure, Privileged context.
Table 1188.
MPU_BAR0,
MPU_BAR1, …,
MPU_BAR6,
MPU_BAR7 Registers
Bits Description Type Reset
31:5 ADDR: This MPU region matches addresses where addr[31:5] (the 27 most
significant bits) are greater than or equal to BAR_ADDR, and less than or equal
to LAR_ADDR.
Readable from any Privileged context, if and only if this region’s S bit is clear,
and MPU_CTRL_NS_HIDE_ADDR is clear. Otherwise readable only from a
Secure, Privileged context.
RW 0x0000000
4:0 Reserved. - -
DMA: MPU_LAR0, MPU_LAR1, …, MPU_LAR6, MPU_LAR7 Registers
Offsets: 0x508, 0x510, …, 0x538, 0x540
Description
Limit address register for MPU region N. Writable only from a Secure, Privileged context, with the exception of the P
bit.
Table 1189.
MPU_LAR0,
MPU_LAR1, …,
MPU_LAR6,
MPU_LAR7 Registers
Bits Description Type Reset
31:5 ADDR: Limit address bits 31:5. Readable from any Privileged context, if and
only if this region’s S bit is clear, and MPU_CTRL_NS_HIDE_ADDR is clear.
Otherwise readable only from a Secure, Privileged context.
RW 0x0000000
4:3 Reserved. - -
RP2350 Datasheet
12.6. DMA 1140
Bits Description Type Reset
2 S: Determines the Secure/Non-secure (=1/0) status of addresses matching
this region, if this region is enabled.
RW 0x0
1 P: Determines the Privileged/Unprivileged (=1/0) status of addresses
matching this region, if this region is enabled. Writable from any Privileged
context, if and only if the S bit is clear. Otherwise, writable only from a Secure,
Privileged context.
RW 0x0
0 EN: Region enable. If 1, any address within range specified by the base
address (BAR_ADDR) and limit address (LAR_ADDR) has the attributes
specified by S and P.
RW 0x0
DMA: CH0_DBG_CTDREQ, CH1_DBG_CTDREQ, …, CH14_DBG_CTDREQ,
CH15_DBG_CTDREQ Registers
Offsets: 0x800, 0x840, …, 0xb80, 0xbc0
Table 1190.
CH0_DBG_CTDREQ,
CH1_DBG_CTDREQ, …,
CH14_DBG_CTDREQ,
CH15_DBG_CTDREQ
Registers
Bits Description Type Reset
31:6 Reserved. - -
5:0 Read: get channel DREQ counter (i.e. how many accesses the DMA expects it
can perform on the peripheral without overflow/underflow. Write any value:
clears the counter, and cause channel to re-initiate DREQ handshake.
WC 0x00
DMA: CH0_DBG_TCR, CH1_DBG_TCR, …, CH14_DBG_TCR, CH15_DBG_TCR
Registers
Offsets: 0x804, 0x844, …, 0xb84, 0xbc4
Table 1191.
CH0_DBG_TCR,
CH1_DBG_TCR, …,
CH14_DBG_TCR,
CH15_DBG_TCR
Registers
Bits Description Type Reset
31:0 Read to get channel TRANS_COUNT reload value, i.e. the length of the next
transfer
RO 0x00000000