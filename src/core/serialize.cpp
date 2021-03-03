#include <fstream>
#include <cstring>
#include "serialize.hpp"
#include "emulator.hpp"

#define VER_MAJOR 0
#define VER_MINOR 0
#define VER_REV 50

using namespace std;

bool Emulator::request_load_state(const char *file_name)
{
    ifstream state(file_name, ios::binary);
    if (!state.is_open())
        return false;
    state.close();
    save_state_path = file_name;
    load_requested = true;
    return true;
}

bool Emulator::request_save_state(const char *file_name)
{
    ofstream state(file_name, ios::binary);
    if (!state.is_open())
        return false;
    state.close();
    save_state_path = file_name;
    save_requested = true;
    return true;
}

void Emulator::load_state(const char *file_name)
{
    load_requested = false;
    printf("[Emulator] Loading state...\n");
    fstream state(file_name, ios::binary | fstream::in);
    if (!state.is_open())
    {
        Errors::non_fatal("Failed to load save state");
        return;
    }

    //Perform sanity checks
    char dobie_buffer[5];
    state.read(dobie_buffer, sizeof(dobie_buffer));
    if (strncmp(dobie_buffer, "DOBIE", 5))
    {
        state.close();
        Errors::non_fatal("Save state invalid");
        return;
    }

    uint32_t major, minor, rev;
    state.read((char*)&major, sizeof(major));
    state.read((char*)&minor, sizeof(minor));
    state.read((char*)&rev, sizeof(rev));

    if (major != VER_MAJOR || minor != VER_MINOR || rev != VER_REV)
    {
        state.close();
        Errors::non_fatal("Save state doesn't match version");
        return;
    }

    StateSerializer ss(state, StateSerializer::Mode::Read);
    do_state(ss);

    reset();

    state.close();
    printf("[Emulator] Success!\n");
}

void Emulator::save_state(const char *file_name)
{
    save_requested = false;

    std::vector<char> data = {};

    printf("[Emulator] Saving state...\n");
    fstream state(file_name, ios::binary | fstream::out | fstream::trunc);
    if (!state.is_open())
    {
        Errors::non_fatal("Failed to save state");
        return;
    }

    uint32_t major = VER_MAJOR;
    uint32_t minor = VER_MINOR;
    uint32_t rev = VER_REV;

    //Sanity check and version
    state << "DOBIE";
    state.write((char*)&major, sizeof(uint32_t));
    state.write((char*)&minor, sizeof(uint32_t));
    state.write((char*)&rev, sizeof(uint32_t));

    StateSerializer ss(state, StateSerializer::Mode::Write);
    do_state(ss);

    state.close();
    printf("Success!\n");
}

void Emulator::do_state(StateSerializer &state)
{
    //Emulator info
    state.Do(&VBLANK_sent);
    state.Do(&frames);

    //RAM
    state.DoBytes(RDRAM, 1024 * 1024 * 32);
    state.DoBytes(IOP_RAM, 1024 * 1024 * 2);
    state.DoBytes(SPU_RAM, 1024 * 1024 * 2);
    state.DoBytes(scratchpad, 1024 * 16);
    state.DoBytes(iop_scratchpad, 1024);
    state.Do(&iop_scratchpad_start);

    //CPUs
    cpu.do_state(state);
    cp0.do_state(state);
    fpu.do_state(state);
    iop.do_state(state);
    vu0.do_state(state);
    vu1.do_state(state);

    //Interrupt registers
    intc.do_state(state);
    iop_intc.do_state(state);

    ////Timers
    //timers.load_state(ss);
    //iop_timers.load_state(ss);

    ////DMA
    //dmac.load_state(ss);
    //iop_dma.load_state(ss);

    ////"Interfaces"
    //gif.load_state(ss);
    //sif.load_state(ss);
    //vif0.load_state(ss);
    //vif1.load_state(ss);

    ////CDVD
    //cdvd.load_state(ss);

    ////GS
    ////Important note - this serialization function is located in gs.cpp as it contains a lot of thread-specific details
    //gs.load_state(ss);

    //scheduler.load_state(ss);
    //pad.load_state(ss);
    //spu.load_state(ss);
    //spu2.load_state(ss);
}


void EmotionEngine::do_state(StateSerializer &state)
{
    state.Do(&cycle_count);
    state.Do(&cycles_to_run);
    state.Do(&icache);
    state.Do(&gpr);
    state.Do(&LO);
    state.Do(&HI);
    state.Do(&LO.hi);
    state.Do(&HI.hi);
    state.Do(&PC);
    state.Do(&new_PC);
    state.Do(&SA);

    //state.read((char*)&IRQ_raised, sizeof(IRQ_raised));
    state.Do(&wait_for_IRQ);
    state.Do(&branch_on);
    state.Do(&delay_slot);

    state.Do(&deci2size);
    state.DoBytes(&deci2handlers, sizeof(Deci2Handler) * deci2size);
}

void Cop0::do_state(StateSerializer &state)
{
    state.DoArray(&gpr, 32);
    state.Do(&status);
    state.Do(&cause);
    state.Do(&EPC);
    state.Do(&ErrorEPC);
    state.Do(&PCCR);
    state.Do(&PCR0);
    state.Do(&PCR1);
    state.DoArray(&tlb, 48);

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
        //Repopulate VTLB
        for (int i = 0; i < 48; i++)
            map_tlb(&tlb[i]);
    }
}

void Cop1::do_state(StateSerializer &state)
{
    //for (int i = 0; i < 32; i++)
    //    state.read((char*)&gpr[i].u, sizeof(uint32_t));
    state.DoArray(&gpr, 32);
    state.Do(&accumulator);
    state.Do(&control);
}

void IOP::do_state(StateSerializer &state)
{
    state.DoArray(&gpr, 32);
    state.Do(&LO);
    state.Do(&HI);
    state.Do(&PC);
    state.Do(&new_PC);
    state.DoArray(&icache, 256);

    state.Do(&branch_delay);
    state.Do(&will_branch);
    state.Do(&wait_for_IRQ);

    //COP0
    state.Do(&cop0.status);
    state.Do(&cop0.cause);
    state.Do(&cop0.EPC);
}

void VectorUnit::do_state(StateSerializer &state)
{
    //for (int i = 0; i < 32; i++)
    //    state.read((char*)&gpr[i].u, sizeof(uint32_t) * 4);
    state.DoArray(&gpr, 32);
    state.DoArray(&int_gpr, 32);
    state.Do(&decoder);

    state.Do(&ACC);
    state.Do(&R);
    state.Do(&I);
    state.Do(&Q);
    state.Do(&P);
    state.Do(&CMSAR0);

    //Pipelines
    state.Do(&new_MAC_flags);
    state.Do(&MAC_pipeline); // TODO: test
    state.Do(&cycle_count);
    state.Do(&finish_DIV_event);
    state.Do(&new_Q_instance); // TODO: union
    state.Do(&DIV_event_started);
    state.Do(&finish_EFU_event);
    state.Do(&new_P_instance); // TODO union
    state.Do(&EFU_event_started);

    state.Do(&int_branch_delay);
    state.Do(&int_backup_reg);
    state.Do(&int_backup_id);

    state.Do(&status);
    state.Do(&status_value);
    state.Do(&status_pipe);
    state.Do(&int_branch_pipeline); // TODO complicated struct
    state.Do(&ILW_pipeline); // TODO array

    state.Do(&pipeline_state); // TODO array

    //XGKICK
    state.Do(&GIF_addr);
    state.Do(&transferring_GIF);
    state.Do(&XGKICK_stall);
    state.Do(&stalled_GIF_addr);

    //Memory
    if (id == 0)
    {
        state.DoBytes(&instr_mem, 1024 * 4);
        state.DoBytes(&data_mem, 1024 * 4);
    }
    else
    {
        state.DoBytes(&instr_mem, 1024 * 16);
        state.DoBytes(&data_mem, 1024 * 16);
    }

    state.Do(&running);
    state.Do(&PC);
    state.Do(&new_PC);
    state.Do(&secondbranch_PC);
    state.Do(&second_branch_pending);
    state.Do(&branch_on);
    state.Do(&branch_on_delay);
    state.Do(&finish_on);
    state.Do(&branch_delay_slot);
    state.Do(&ebit_delay_slot);
}

void INTC::do_state(StateSerializer &state)
{
    state.Do(&INTC_MASK);
    state.Do(&INTC_STAT);
    state.Do(&stat_speedhack_active);
    state.Do(&read_stat_count);
}

void IOP_INTC::do_state(StateSerializer &state)
{
    state.Do(&I_CTRL);
    state.Do(&I_STAT);
    state.Do(&I_MASK);
}

void EmotionTiming::do_state(StateSerializer &state)
{
    state.Do(&timers);
    state.Do(&events);
}

void IOPTiming::do_state(StateSerializer &state)
{
    state.DoArray(&timers, 6);
}

void DMAC::load_state(ifstream &state)
{
    state.read((char*)&channels, sizeof(channels));

    apply_dma_funcs();

    state.read((char*)&control, sizeof(control));
    state.read((char*)&interrupt_stat, sizeof(interrupt_stat));
    state.read((char*)&PCR, sizeof(PCR));
    state.read((char*)&RBOR, sizeof(RBOR));
    state.read((char*)&RBSR, sizeof(RBSR));
    state.read((char*)&SQWC, sizeof(SQWC));
    state.read((char*)&STADR, sizeof(STADR));
    state.read((char*)&mfifo_empty_triggered, sizeof(mfifo_empty_triggered));
    state.read((char*)&cycles_to_run, sizeof(cycles_to_run));
    state.read((char*)&master_disable, sizeof(master_disable));

    int index;
    state.read((char*)&index, sizeof(index));
    if (index >= 0)
        active_channel = &channels[index];
    else
        active_channel = nullptr;

    int queued_size;
    state.read((char*)&queued_size, sizeof(queued_size));
    if (queued_size > 0)
    {
        for (int i = 0; i < queued_size; i++)
        {
            state.read((char*)&index, sizeof(index));
            queued_channels.push_back(&channels[index]);
        }
    }
}

void DMAC::save_state(ofstream &state)
{
    state.write((char*)&channels, sizeof(channels));

    state.write((char*)&control, sizeof(control));
    state.write((char*)&interrupt_stat, sizeof(interrupt_stat));
    state.write((char*)&PCR, sizeof(PCR));
    state.write((char*)&RBOR, sizeof(RBOR));
    state.write((char*)&RBSR, sizeof(RBSR));
    state.write((char*)&SQWC, sizeof(SQWC));
    state.write((char*)&STADR, sizeof(STADR));
    state.write((char*)&mfifo_empty_triggered, sizeof(mfifo_empty_triggered));
    state.write((char*)&cycles_to_run, sizeof(cycles_to_run));
    state.write((char*)&master_disable, sizeof(master_disable));

    int index;
    if (active_channel)
        index = active_channel->index;
    else
        index = -1;

    state.write((char*)&index, sizeof(index));
    int size = queued_channels.size();
    state.write((char*)&size, sizeof(size));
    if (size > 0)
    {
        for (auto it = queued_channels.begin(); it != queued_channels.end(); it++ )
        {
            index = (*it)->index;
            state.write((char*)&index, sizeof(index));
        }
    }
}

void IOP_DMA::load_state(ifstream &state)
{
    state.read((char*)&channels, sizeof(channels));

    int active_index;
    state.read((char*)&active_index, sizeof(active_index));
    if (active_index)
        active_channel = &channels[active_index - 1];
    else
        active_channel = nullptr;

    int queued_size = 0;
    state.read((char*)&queued_size, sizeof(queued_size));
    for (int i = 0; i < queued_size; i++)
    {
        int index;
        state.read((char*)&index, sizeof(index));
        queued_channels.push_back(&channels[index]);
    }

    state.read((char*)&DPCR, sizeof(DPCR));
    state.read((char*)&DICR, sizeof(DICR));

    //We have to reapply the function pointers as there's no guarantee they will remain in memory
    //the next time Dobie is loaded
    apply_dma_functions();
}

void IOP_DMA::save_state(ofstream &state)
{
    state.write((char*)&channels, sizeof(channels));

    int active_index = 0;
    if (active_channel)
        active_index = active_channel->index + 1;
    state.write((char*)&active_index, sizeof(active_index));

    int queued_size = queued_channels.size();
    state.write((char*)&queued_size, sizeof(queued_size));
    for (auto it = queued_channels.begin(); it != queued_channels.end(); it++)
    {
        IOP_DMA_Channel* chan = *it;
        state.write((char*)&chan->index, sizeof(chan->index));
    }

    state.write((char*)&DPCR, sizeof(DPCR));
    state.write((char*)&DICR, sizeof(DICR));
}

void GraphicsInterface::load_state(ifstream &state)
{
    int size;
    uint128_t FIFO_buffer[16];
    state.read((char*)&size, sizeof(size));
    state.read((char*)&FIFO_buffer, sizeof(uint128_t) * size);
    for (int i = 0; i < size; i++)
        FIFO.push(FIFO_buffer[i]);

    state.read((char*)&path, sizeof(path));
    state.read((char*)&active_path, sizeof(active_path));
    state.read((char*)&path_queue, sizeof(path_queue));
    state.read((char*)&path3_vif_masked, sizeof(path3_vif_masked));
    state.read((char*)&internal_Q, sizeof(internal_Q));
    state.read((char*)&path3_dma_running, sizeof(path3_dma_running));
    state.read((char*)&intermittent_mode, sizeof(intermittent_mode));
    state.read((char*)&outputting_path, sizeof(outputting_path));
    state.read((char*)&path3_mode_masked, sizeof(path3_mode_masked));
    state.read((char*)&gif_temporary_stop, sizeof(gif_temporary_stop));
}

void GraphicsInterface::save_state(ofstream &state)
{
    int size = FIFO.size();
    uint128_t FIFO_buffer[16];
    for (int i = 0; i < size; i++)
    {
        FIFO_buffer[i] = FIFO.front();
        FIFO.pop();
    }
    state.write((char*)&size, sizeof(size));
    state.write((char*)&FIFO_buffer, sizeof(uint128_t) * size);
    for (int i = 0; i < size; i++)
        FIFO.push(FIFO_buffer[i]);

    state.write((char*)&path, sizeof(path));
    state.write((char*)&active_path, sizeof(active_path));
    state.write((char*)&path_queue, sizeof(path_queue));
    state.write((char*)&path3_vif_masked, sizeof(path3_vif_masked));
    state.write((char*)&internal_Q, sizeof(internal_Q));
    state.write((char*)&path3_dma_running, sizeof(path3_dma_running));
    state.write((char*)&intermittent_mode, sizeof(intermittent_mode));
    state.write((char*)&outputting_path, sizeof(outputting_path));
    state.write((char*)&path3_mode_masked, sizeof(path3_mode_masked));
    state.write((char*)&gif_temporary_stop, sizeof(gif_temporary_stop));
}

void SubsystemInterface::load_state(ifstream &state)
{
    state.read((char*)&mscom, sizeof(mscom));
    state.read((char*)&smcom, sizeof(smcom));
    state.read((char*)&msflag, sizeof(msflag));
    state.read((char*)&smflag, sizeof(smflag));
    state.read((char*)&control, sizeof(control));

    int size;
    uint32_t buffer[32];
    state.read((char*)&size, sizeof(int));
    state.read((char*)&buffer, sizeof(uint32_t) * size);

    //FIFOs are already cleared by the reset call, so no need to pop them
    for (int i = 0; i < size; i++)
        SIF0_FIFO.push(buffer[i]);

    state.read((char*)&size, sizeof(int));
    state.read((char*)&buffer, sizeof(uint32_t) * size);

    for (int i = 0; i < size; i++)
        SIF1_FIFO.push(buffer[i]);
}

void SubsystemInterface::save_state(ofstream &state)
{
    state.write((char*)&mscom, sizeof(mscom));
    state.write((char*)&smcom, sizeof(smcom));
    state.write((char*)&msflag, sizeof(msflag));
    state.write((char*)&smflag, sizeof(smflag));
    state.write((char*)&control, sizeof(control));

    int size = SIF0_FIFO.size();
    uint32_t buffer[32];
    for (int i = 0; i < size; i++)
    {
        buffer[i] = SIF0_FIFO.front();
        SIF0_FIFO.pop();
    }
    state.write((char*)&size, sizeof(int));
    state.write((char*)&buffer, sizeof(uint32_t) * size);
    for (int i = 0; i < size; i++)
        SIF0_FIFO.push(buffer[i]);

    size = SIF1_FIFO.size();
    for (int i = 0; i < size; i++)
    {
        buffer[i] = SIF1_FIFO.front();
        SIF1_FIFO.pop();
    }
    state.write((char*)&size, sizeof(int));
    state.write((char*)&buffer, sizeof(uint32_t) * size);
    for (int i = 0; i < size; i++)
        SIF1_FIFO.push(buffer[i]);
}

void VectorInterface::load_state(ifstream &state)
{
    int size, internal_size;
    uint32_t FIFO_buffer[64];
    state.read((char*)&size, sizeof(size));
    state.read((char*)&FIFO_buffer, sizeof(uint32_t) * size);
    for (int i = 0; i < size; i++)
        FIFO.push(FIFO_buffer[i]);

    state.read((char*)&internal_size, sizeof(internal_size));
    state.read((char*)&FIFO_buffer, sizeof(uint32_t) * internal_size);
    for (int i = 0; i < internal_size; i++)
        internal_FIFO.push(FIFO_buffer[i]);

    state.read((char*)&imm, sizeof(imm));
    state.read((char*)&command, sizeof(command));
    state.read((char*)&mpg, sizeof(mpg));
    state.read((char*)&unpack, sizeof(unpack));
    state.read((char*)&wait_for_VU, sizeof(wait_for_VU));
    state.read((char*)&flush_stall, sizeof(flush_stall));
    state.read((char*)&wait_cmd_value, sizeof(wait_cmd_value));

    state.read((char*)&buffer_size, sizeof(buffer_size));
    state.read((char*)&buffer, sizeof(buffer));

    state.read((char*)&DBF, sizeof(DBF));
    state.read((char*)&CYCLE, sizeof(CYCLE));
    state.read((char*)&OFST, sizeof(OFST));
    state.read((char*)&BASE, sizeof(BASE));
    state.read((char*)&TOP, sizeof(TOP));
    state.read((char*)&TOPS, sizeof(TOPS));
    state.read((char*)&ITOP, sizeof(ITOP));
    state.read((char*)&ITOPS, sizeof(ITOPS));
    state.read((char*)&MODE, sizeof(MODE));
    state.read((char*)&MASK, sizeof(MASK));
    state.read((char*)&ROW, sizeof(ROW));
    state.read((char*)&COL, sizeof(COL));
    state.read((char*)&CODE, sizeof(CODE));
    state.read((char*)&command_len, sizeof(command_len));

    state.read((char*)&vif_ibit_detected, sizeof(vif_ibit_detected));
    state.read((char*)&vif_interrupt, sizeof(vif_interrupt));
    state.read((char*)&vif_stalled, sizeof(vif_stalled));
    state.read((char*)&vif_stop, sizeof(vif_stop));
    state.read((char*)&vif_forcebreak, sizeof(vif_forcebreak));
    state.read((char*)&vif_cmd_status, sizeof(vif_cmd_status));
    state.read((char*)&internal_WL, sizeof(internal_WL));

    state.read((char*)&mark_detected, sizeof(mark_detected));
    state.read((char*)&VIF_ERR, sizeof(VIF_ERR));
}

void VectorInterface::save_state(ofstream &state)
{
    int size = FIFO.size();
    int internal_size = internal_FIFO.size();
    uint32_t FIFO_buffer[64];
    for (int i = 0; i < size; i++)
    {
        FIFO_buffer[i] = FIFO.front();
        FIFO.pop();
    }
    state.write((char*)&size, sizeof(size));
    state.write((char*)&FIFO_buffer, sizeof(uint32_t) * size);
    for (int i = 0; i < size; i++)
        FIFO.push(FIFO_buffer[i]);

    for (int i = 0; i < internal_size; i++)
    {
        FIFO_buffer[i] = internal_FIFO.front();
        internal_FIFO.pop();
    }
    state.write((char*)&internal_size, sizeof(internal_size));
    state.write((char*)&FIFO_buffer, sizeof(uint32_t) * internal_size);
    for (int i = 0; i < internal_size; i++)
        internal_FIFO.push(FIFO_buffer[i]);

    state.write((char*)&imm, sizeof(imm));
    state.write((char*)&command, sizeof(command));
    state.write((char*)&mpg, sizeof(mpg));
    state.write((char*)&unpack, sizeof(unpack));
    state.write((char*)&wait_for_VU, sizeof(wait_for_VU));
    state.write((char*)&flush_stall, sizeof(flush_stall));
    state.write((char*)&wait_cmd_value, sizeof(wait_cmd_value));

    state.write((char*)&buffer_size, sizeof(buffer_size));
    state.write((char*)&buffer, sizeof(buffer));

    state.write((char*)&DBF, sizeof(DBF));
    state.write((char*)&CYCLE, sizeof(CYCLE));
    state.write((char*)&OFST, sizeof(OFST));
    state.write((char*)&BASE, sizeof(BASE));
    state.write((char*)&TOP, sizeof(TOP));
    state.write((char*)&TOPS, sizeof(TOPS));
    state.write((char*)&ITOP, sizeof(ITOP));
    state.write((char*)&ITOPS, sizeof(ITOPS));
    state.write((char*)&MODE, sizeof(MODE));
    state.write((char*)&MASK, sizeof(MASK));
    state.write((char*)&ROW, sizeof(ROW));
    state.write((char*)&COL, sizeof(COL));
    state.write((char*)&CODE, sizeof(CODE));
    state.write((char*)&command_len, sizeof(command_len));

    state.write((char*)&vif_ibit_detected, sizeof(vif_ibit_detected));
    state.write((char*)&vif_interrupt, sizeof(vif_interrupt));
    state.write((char*)&vif_stalled, sizeof(vif_stalled));
    state.write((char*)&vif_stop, sizeof(vif_stop));
    state.write((char*)&vif_forcebreak, sizeof(vif_forcebreak));
    state.write((char*)&vif_cmd_status, sizeof(vif_cmd_status));
    state.write((char*)&internal_WL, sizeof(internal_WL));

    state.write((char*)&mark_detected, sizeof(mark_detected));
    state.write((char*)&VIF_ERR, sizeof(VIF_ERR));
}

void CDVD_Drive::load_state(ifstream &state)
{
    state.read((char*)&file_size, sizeof(file_size));
    state.read((char*)&read_bytes_left, sizeof(read_bytes_left));
    state.read((char*)&disc_type, sizeof(disc_type));
    state.read((char*)&speed, sizeof(speed));
    state.read((char*)&current_sector, sizeof(current_sector));
    state.read((char*)&sector_pos, sizeof(sector_pos));
    state.read((char*)&sectors_left, sizeof(sectors_left));
    state.read((char*)&block_size, sizeof(block_size));
    state.read((char*)&read_buffer, sizeof(read_buffer));
    state.read((char*)&ISTAT, sizeof(ISTAT));
    state.read((char*)&drive_status, sizeof(drive_status));
    state.read((char*)&is_spinning, sizeof(is_spinning));

    state.read((char*)&active_N_command, sizeof(active_N_command));
    state.read((char*)&N_command, sizeof(N_command));
    state.read((char*)&N_command_params, sizeof(N_command_params));
    state.read((char*)&N_params, sizeof(N_params));
    state.read((char*)&N_status, sizeof(N_status));

    state.read((char*)&S_command, sizeof(S_command));
    state.read((char*)&S_command_params, sizeof(S_command_params));
    state.read((char*)&S_outdata, sizeof(S_outdata));
    state.read((char*)&S_params, sizeof(S_params));
    state.read((char*)&S_out_params, sizeof(S_out_params));
    state.read((char*)&S_status, sizeof(S_status));
    state.read((char*)&rtc, sizeof(rtc));
}

void CDVD_Drive::save_state(ofstream &state)
{
    state.write((char*)&file_size, sizeof(file_size));
    state.write((char*)&read_bytes_left, sizeof(read_bytes_left));
    state.write((char*)&disc_type, sizeof(disc_type));
    state.write((char*)&speed, sizeof(speed));
    state.write((char*)&current_sector, sizeof(current_sector));
    state.write((char*)&sector_pos, sizeof(sector_pos));
    state.write((char*)&sectors_left, sizeof(sectors_left));
    state.write((char*)&block_size, sizeof(block_size));
    state.write((char*)&read_buffer, sizeof(read_buffer));
    state.write((char*)&ISTAT, sizeof(ISTAT));
    state.write((char*)&drive_status, sizeof(drive_status));
    state.write((char*)&is_spinning, sizeof(is_spinning));

    state.write((char*)&active_N_command, sizeof(active_N_command));
    state.write((char*)&N_command, sizeof(N_command));
    state.write((char*)&N_command_params, sizeof(N_command_params));
    state.write((char*)&N_params, sizeof(N_params));
    state.write((char*)&N_status, sizeof(N_status));

    state.write((char*)&S_command, sizeof(S_command));
    state.write((char*)&S_command_params, sizeof(S_command_params));
    state.write((char*)&S_outdata, sizeof(S_outdata));
    state.write((char*)&S_params, sizeof(S_params));
    state.write((char*)&S_out_params, sizeof(S_out_params));
    state.write((char*)&S_status, sizeof(S_status));
    state.write((char*)&rtc, sizeof(rtc));
}

void Scheduler::load_state(ifstream &state)
{
    state.read((char*)&ee_cycles, sizeof(ee_cycles));
    state.read((char*)&bus_cycles, sizeof(bus_cycles));
    state.read((char*)&iop_cycles, sizeof(iop_cycles));
    state.read((char*)&run_cycles, sizeof(run_cycles));
    state.read((char*)&closest_event_time, sizeof(closest_event_time));

    events.clear();

    int event_size = 0;
    state.read((char*)&event_size, sizeof(event_size));

    for (int i = 0; i < event_size; i++)
    {
        SchedulerEvent event;
        state.read((char*)&event, sizeof(event));

        events.push_back(event);
    }

    state.read((char*)&next_event_id, sizeof(next_event_id));

    int timer_size = 0;
    state.read((char*)&timer_size, sizeof(timer_size));

    timers.clear();

    for (int i = 0; i < timer_size; i++)
    {
        SchedulerTimer timer;
        state.read((char*)&timer, sizeof(timer));

        timers.push_back(timer);
    }
}

void Scheduler::save_state(ofstream &state)
{
    state.write((char*)&ee_cycles, sizeof(ee_cycles));
    state.write((char*)&bus_cycles, sizeof(bus_cycles));
    state.write((char*)&iop_cycles, sizeof(iop_cycles));
    state.write((char*)&run_cycles, sizeof(run_cycles));
    state.write((char*)&closest_event_time, sizeof(closest_event_time));

    int event_size = events.size();
    state.write((char*)&event_size, sizeof(event_size));

    for (auto it = events.begin(); it != events.end(); it++)
    {
        SchedulerEvent event = *it;
        state.write((char*)&event, sizeof(event));
    }

    state.write((char*)&next_event_id, sizeof(next_event_id));

    int timer_size = timers.size();
    state.write((char*)&timer_size, sizeof(timer_size));

    for (int i = 0; i < timer_size; i++)
        state.write((char*)&timers[i], sizeof(SchedulerTimer));
}

void Gamepad::load_state(ifstream &state)
{
    state.read((char*)&command_buffer, sizeof(command_buffer));
    state.read((char*)&rumble_values, sizeof(rumble_values));
    state.read((char*)&mode_lock, sizeof(mode_lock));
    state.read((char*)&command, sizeof(command));
    state.read((char*)&command_length, sizeof(command_length));
    state.read((char*)&data_count, sizeof(data_count));
    state.read((char*)&pad_mode, sizeof(pad_mode));
    state.read((char*)&config_mode, sizeof(config_mode));
}

void Gamepad::save_state(ofstream &state)
{
    state.write((char*)&command_buffer, sizeof(command_buffer));
    state.write((char*)&rumble_values, sizeof(rumble_values));
    state.write((char*)&mode_lock, sizeof(mode_lock));
    state.write((char*)&command, sizeof(command));
    state.write((char*)&command_length, sizeof(command_length));
    state.write((char*)&data_count, sizeof(data_count));
    state.write((char*)&pad_mode, sizeof(pad_mode));
    state.write((char*)&config_mode, sizeof(config_mode));
}

void SPU::load_state(ifstream &state)
{
    state.read((char*)&voices, sizeof(voices));
    state.read((char*)&core_att, sizeof(core_att));
    state.read((char*)&status, sizeof(status));
    state.read((char*)&spdif_irq, sizeof(spdif_irq));
    state.read((char*)&transfer_addr, sizeof(transfer_addr));
    state.read((char*)&current_addr, sizeof(current_addr));
    state.read((char*)&autodma_ctrl, sizeof(autodma_ctrl));
    state.read((char*)&buffer_pos, sizeof(buffer_pos));
    state.read((char*)&IRQA, sizeof(IRQA));
    state.read((char*)&ENDX, sizeof(ENDX));
    state.read((char*)&key_off, sizeof(key_off));
    state.read((char*)&key_on, sizeof(key_on));
    state.read((char*)&noise, sizeof(noise));
    state.read((char*)&output_enable, sizeof(output_enable));

    state.read((char*)&reverb, sizeof(reverb));
    state.read((char*)&effect_enable, sizeof(effect_enable));
    state.read((char*)&effect_volume_l, sizeof(effect_volume_l));
    state.read((char*)&effect_volume_r, sizeof(effect_volume_r));

    state.read((char*)&current_buffer, sizeof(current_buffer));
    state.read((char*)&ADMA_progress, sizeof(ADMA_progress));
    state.read((char*)&data_input_volume_l, sizeof(data_input_volume_l));
    state.read((char*)&data_input_volume_r, sizeof(data_input_volume_r));
    state.read((char*)&core_volume_l, sizeof(core_volume_l));
    state.read((char*)&core_volume_r, sizeof(core_volume_r));
    state.read((char*)&MVOLL, sizeof(MVOLL));
    state.read((char*)&MVOLR, sizeof(MVOLR));

    state.read((char*)&mix_state, sizeof(mix_state));
    state.read((char*)&voice_mixdry_left, sizeof(voice_mixdry_left));
    state.read((char*)&voice_mixdry_right, sizeof(voice_mixdry_right));
    state.read((char*)&voice_mixwet_left, sizeof(voice_mixwet_left));
    state.read((char*)&voice_mixwet_right, sizeof(voice_mixwet_right));
    state.read((char*)&voice_pitch_mod, sizeof(voice_pitch_mod));
    state.read((char*)&voice_noise_gen, sizeof(voice_noise_gen));
}

void SPU::save_state(ofstream &state)
{
    state.write((char*)&voices, sizeof(voices));
    state.write((char*)&core_att, sizeof(core_att));
    state.write((char*)&status, sizeof(status));
    state.write((char*)&spdif_irq, sizeof(spdif_irq));
    state.write((char*)&transfer_addr, sizeof(transfer_addr));
    state.write((char*)&current_addr, sizeof(current_addr));
    state.write((char*)&autodma_ctrl, sizeof(autodma_ctrl));
    state.write((char*)&buffer_pos, sizeof(buffer_pos));
    state.write((char*)&IRQA, sizeof(IRQA));
    state.write((char*)&ENDX, sizeof(ENDX));
    state.write((char*)&key_off, sizeof(key_off));
    state.write((char*)&key_on, sizeof(key_on));
    state.write((char*)&noise, sizeof(noise));
    state.write((char*)&output_enable, sizeof(output_enable));

    state.write((char*)&reverb, sizeof(reverb));
    state.write((char*)&effect_enable, sizeof(effect_enable));
    state.write((char*)&effect_volume_l, sizeof(effect_volume_l));
    state.write((char*)&effect_volume_r, sizeof(effect_volume_r));

    state.write((char*)&current_buffer, sizeof(current_buffer));
    state.write((char*)&ADMA_progress, sizeof(ADMA_progress));
    state.write((char*)&data_input_volume_l, sizeof(data_input_volume_l));
    state.write((char*)&data_input_volume_r, sizeof(data_input_volume_r));
    state.write((char*)&core_volume_l, sizeof(core_volume_l));
    state.write((char*)&core_volume_r, sizeof(core_volume_r));
    state.write((char*)&MVOLL, sizeof(MVOLL));
    state.write((char*)&MVOLR, sizeof(MVOLR));

    state.write((char*)&mix_state, sizeof(mix_state));
    state.write((char*)&voice_mixdry_left, sizeof(voice_mixdry_left));
    state.write((char*)&voice_mixdry_right, sizeof(voice_mixdry_right));
    state.write((char*)&voice_mixwet_left, sizeof(voice_mixwet_left));
    state.write((char*)&voice_mixwet_right, sizeof(voice_mixwet_right));
    state.write((char*)&voice_pitch_mod, sizeof(voice_pitch_mod));
    state.write((char*)&voice_noise_gen, sizeof(voice_noise_gen));
}
