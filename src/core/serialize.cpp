#include "serialize.hpp"
#include "emulator.hpp"
#include <cstring>
#include <fstream>

#define VER_MAJOR 0
#define VER_MINOR 0
#define VER_REV 50

using namespace std;

bool Emulator::request_load_state(const char* file_name)
{
    ifstream state(file_name, ios::binary);
    if (!state.is_open())
        return false;
    state.close();
    save_state_path = file_name;
    load_requested = true;
    return true;
}

bool Emulator::request_save_state(const char* file_name)
{
    ofstream state(file_name, ios::binary);
    if (!state.is_open())
        return false;
    state.close();
    save_state_path = file_name;
    save_requested = true;
    return true;
}

void Emulator::load_state(const char* file_name)
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

void Emulator::save_state(const char* file_name)
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

void Emulator::do_state(StateSerializer& state)
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
    cdvd.do_state(state);

    ////GS
    ////Important note - this serialization function is located in gs.cpp as it contains a lot of thread-specific details
    //gs.load_state(ss);

    //scheduler.load_state(ss);
    pad.do_state(state);
    spu.do_state(state);
    spu2.do_state(state);
}

void EmotionEngine::do_state(StateSerializer& state)
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
    //state.DoBytes(&deci2handlers, sizeof(Deci2Handler) * deci2size);
    state.DoArray(&deci2handlers, 128);
}

void Cop0::do_state(StateSerializer& state)
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

void Cop1::do_state(StateSerializer& state)
{
    //for (int i = 0; i < 32; i++)
    //    state.read((char*)&gpr[i].u, sizeof(uint32_t));
    state.DoArray(&gpr, 32);
    state.Do(&accumulator);
    state.Do(&control);
}

void IOP::do_state(StateSerializer& state)
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

void VectorUnit::do_state(StateSerializer& state)
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
    state.Do(&ILW_pipeline);        // TODO array

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

void INTC::do_state(StateSerializer& state)
{
    state.Do(&INTC_MASK);
    state.Do(&INTC_STAT);
    state.Do(&stat_speedhack_active);
    state.Do(&read_stat_count);
}

void IOP_INTC::do_state(StateSerializer& state)
{
    state.Do(&I_CTRL);
    state.Do(&I_STAT);
    state.Do(&I_MASK);
}

void EmotionTiming::do_state(StateSerializer& state)
{
    state.Do(&timers);
    state.Do(&events);
}

void IOPTiming::do_state(StateSerializer& state)
{
    state.DoArray(&timers, 6);
}

void DMAC::load_state(StateSerializer& state)
{
    state.Do(&channels); // TODO array

    if (state.GetMode() == StateSerializer::Mode::Read)
        apply_dma_funcs();

    state.Do(&control);
    state.Do(&interrupt_stat);
    state.Do(&PCR);
    state.Do(&RBOR);
    state.Do(&RBSR);
    state.Do(&SQWC);
    state.Do(&STADR);
    state.Do(&mfifo_empty_triggered);
    state.Do(&cycles_to_run);
    state.Do(&master_disable);

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
        // TODO Blergh
        int index;
        state.Do(&index);
        if (index >= 0)
            active_channel = &channels[index];
        else
            active_channel = nullptr;
    }
    else
    {
        int index;
        if (active_channel)
            index = active_channel->index;
        else
            index = -1;

        state.Do(&index);
    }

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
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
    else
    {
        int size = queued_channels.size();
        state.write((char*)&size, sizeof(size));
        if (size > 0)
        {
            for (auto it = queued_channels.begin(); it != queued_channels.end(); it++)
            {
                index = (*it)->index;
                state.write((char*)&index, sizeof(index));
            }
        }
    }
}

void IOP_DMA::load_state(ifstream& state)
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

void IOP_DMA::save_state(ofstream& state)
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

void GraphicsInterface::load_state(ifstream& state)
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

void GraphicsInterface::save_state(ofstream& state)
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

void SubsystemInterface::load_state(ifstream& state)
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

void SubsystemInterface::save_state(ofstream& state)
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

void VectorInterface::load_state(ifstream& state)
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

void VectorInterface::save_state(ofstream& state)
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

void CDVD_Drive::do_state(StateSerializer& state)
{
    state.Do(&file_size);
    state.Do(&read_bytes_left);
    state.Do(&disc_type);
    state.Do(&speed);
    state.Do(&current_sector);
    state.Do(&sector_pos);
    state.Do(&sectors_left);
    state.Do(&block_size);
    state.Do(&read_buffer);
    state.Do(&ISTAT);
    state.Do(&drive_status);
    state.Do(&is_spinning);

    state.Do(&active_N_command);
    state.Do(&N_command);
    state.Do(&N_command_params);
    state.Do(&N_params);
    state.Do(&N_status);

    state.Do(&S_command);
    state.Do(&S_command_params);
    state.Do(&S_outdata);
    state.Do(&S_params);
    state.Do(&S_out_params);
    state.Do(&S_status);
    state.Do(&rtc);
}

void Scheduler::load_state(ifstream& state)
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

void Scheduler::save_state(ofstream& state)
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

void Gamepad::do_state(StateSerializer& state)
{
    state.Do(&command_buffer);
    state.Do(&rumble_values);
    state.Do(&mode_lock);
    state.Do(&command);
    state.Do(&command_length);
    state.Do(&data_count);
    state.Do(&pad_mode);
    state.Do(&config_mode);
}

void SPU::do_state(StateSerializer& state)
{
    state.Do(&voices);
    state.Do(&core_att);
    state.Do(&status);
    state.Do(&spdif_irq);
    state.Do(&transfer_addr);
    state.Do(&current_addr);
    state.Do(&autodma_ctrl);
    state.Do(&buffer_pos);
    state.Do(&IRQA);
    state.Do(&ENDX);
    state.Do(&key_off);
    state.Do(&key_on);
    state.Do(&noise);
    state.Do(&output_enable);
    state.Do(&reverb);
    state.Do(&effect_enable);
    state.Do(&effect_volume_l);
    state.Do(&effect_volume_r);
    state.Do(&current_buffer);
    state.Do(&ADMA_progress);
    state.Do(&data_input_volume_l);
    state.Do(&data_input_volume_r);
    state.Do(&core_volume_l);
    state.Do(&core_volume_r);
    state.Do(&MVOLL);
    state.Do(&MVOLR);
    state.Do(&mix_state);
    state.Do(&voice_mixdry_left);
    state.Do(&voice_mixdry_right);
    state.Do(&voice_mixwet_left);
    state.Do(&voice_mixwet_right);
    state.Do(&voice_pitch_mod);
    state.Do(&voice_noise_gen);
}
