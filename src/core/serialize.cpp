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

    reset();

    StateSerializer ss(state, StateSerializer::Mode::Read);
    do_state(ss);

    state.close();
    printf("[Emulator] Success!\n");
}

void Emulator::save_state(const char* file_name)
{
    save_requested = false;

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
    timers.do_state(state);
    iop_timers.do_state(state);

    ////DMA
    dmac.do_state(state);
    iop_dma.do_state(state);

    ////"Interfaces"
    gif.do_state(state);
    sif.do_state(state);
    vif0.do_state(state);
    vif1.do_state(state);

    ////CDVD
    cdvd.do_state(state);

    ////GS
    ////Important note - this serialization function is located in gs.cpp as it contains a lot of thread-specific details
    gs.do_state(state);

    scheduler.do_state(state);
    pad.do_state(state);
    spu.do_state(state);
    spu2.do_state(state);
}

void EmotionEngine::do_state(StateSerializer& state)
{
    state.Do(&cycle_count);
    state.Do(&cycles_to_run);
    state.DoArray(icache, 128);
    state.DoArray(gpr, 512);
    state.Do(&LO);
    state.Do(&HI);
    state.Do(&LO.hi);
    state.Do(&HI.hi);
    state.Do(&PC);
    state.Do(&new_PC);
    state.Do(&SA);

    state.Do(&wait_for_IRQ);
    state.Do(&branch_on);
    state.Do(&delay_slot);

    state.Do(&deci2size);
    //state.DoBytes(&deci2handlers, sizeof(Deci2Handler) * deci2size);
    state.DoArray(deci2handlers, 128);
}

void Cop0::do_state(StateSerializer& state)
{
    state.DoArray(gpr, 32);
    state.Do(&status);
    state.Do(&cause);
    state.Do(&EPC);
    state.Do(&ErrorEPC);
    state.Do(&PCCR);
    state.Do(&PCR0);
    state.Do(&PCR1);
    state.DoArray(tlb, 48);

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
    state.DoArray(gpr, 32);
    state.Do(&accumulator);
    state.Do(&control);
}

void IOP::do_state(StateSerializer& state)
{
    state.DoArray(gpr, 32);
    state.Do(&LO);
    state.Do(&HI);
    state.Do(&PC);
    state.Do(&new_PC);
    state.DoArray(icache, 256);

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
    state.DoArray(gpr, 32);
    state.DoArray(int_gpr, 16);
    state.Do(&decoder);

    state.Do(&ACC);
    state.Do(&R);
    state.Do(&I);
    state.Do(&Q);
    state.Do(&P);
    state.Do(&CMSAR0);

    //Pipelines
    state.Do(&new_MAC_flags);
    state.DoArray(MAC_pipeline, 4); // TODO: test
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
    state.DoArray(ILW_pipeline, 4);        // TODO array

    state.DoArray(pipeline_state, 2); // TODO array

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
    state.DoArray(timers, 4);
    state.DoArray(events, 4);
}

void IOPTiming::do_state(StateSerializer& state)
{
    state.DoArray(timers, 6);
}

void DMAC::do_state(StateSerializer& state)
{
    state.DoArray(channels, 15); // TODO array

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

    int index = -1;

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
        // TODO Blergh
        state.Do(&index);
        if (index >= 0)
            active_channel = &channels[index];
        else
            active_channel = nullptr;
    }
    else
    {
        if (active_channel)
            index = active_channel->index;
        else
            index = -1;

        state.Do(&index);
    }

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
        int queued_size;
        state.Do(&queued_size);
        if (queued_size > 0)
        {
            for (int i = 0; i < queued_size; i++)
            {
                state.Do(&index);
                queued_channels.push_back(&channels[index]);
            }
        }
    }
    else
    {
        int size = queued_channels.size();
        state.Do(&size);
        if (size > 0)
        {
            for (auto it = queued_channels.begin(); it != queued_channels.end(); it++)
            {
                index = (*it)->index;
                state.Do((&index));
            }
        }
    }
}

void IOP_DMA::do_state(StateSerializer& state)
{
    state.Do(&channels);

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
        int active_index;
        state.Do(&active_index);
        if (active_index)
            active_channel = &channels[active_index - 1];
        else
            active_channel = nullptr;
    }
    else
    {
        int active_index = 0;
        if (active_channel)
            active_index = active_channel->index + 1;
        state.Do(&active_index);
    }

    if (state.GetMode() == StateSerializer::Mode::Read)
    {
        int queued_size = 0;
        state.Do(&queued_size);
        for (int i = 0; i < queued_size; i++)
        {
            int index;
            state.Do(&index);
            queued_channels.push_back(&channels[index]);
        }
    }
    else
    {
        int queued_size = static_cast<int>(queued_channels.size());
        state.Do(&queued_size);
        for (auto it = queued_channels.begin(); it != queued_channels.end(); it++)
        {
            IOP_DMA_Channel* chan = *it;
            state.Do(&chan->index);
        }
    }

    state.Do(&DPCR);
    state.Do(&DICR);

    //We have to reapply the function pointers as there's no guarantee they will remain in memory
    //the next time Dobie is loaded
    if (state.GetMode() == StateSerializer::Mode::Read)
        apply_dma_functions();
}

void GraphicsInterface::do_state(StateSerializer& state)
{
    //if (state.GetMode() == StateSerializer::Mode::Read)
    //{
    //    int size;
    //    uint128_t FIFO_buffer[16];
    //    state.Do(&size);
    //    state.DoBytes(&FIFO_buffer, sizeof(uint128_t) * size);
    //    for (int i = 0; i < size; i++)
    //        FIFO.push(FIFO_buffer[i]);
    //}
    //else
    //{
    //    int size = static_cast<int>(FIFO.size());
    //    uint128_t FIFO_buffer[16];
    //    for (int i = 0; i < size; i++)
    //    {
    //        FIFO_buffer[i] = FIFO.front();
    //        FIFO.pop();
    //    }
    //    state.Do(&size);
    //    state.DoBytes(&FIFO_buffer, sizeof(uint128_t) * size);
    //    for (int i = 0; i < size; i++)
    //        FIFO.push(FIFO_buffer[i]);
    //}
    state.Do(&FIFO);

    state.Do(&path);
    state.Do(&active_path);
    state.Do(&path_queue);
    state.Do(&path3_vif_masked);
    state.Do(&internal_Q);
    state.Do(&path3_dma_running);
    state.Do(&intermittent_mode);
    state.Do(&outputting_path);
    state.Do(&path3_mode_masked);
    state.Do(&gif_temporary_stop);
}

void SubsystemInterface::do_state(StateSerializer& state)
{
    state.Do(&mscom);
    state.Do(&smcom);
    state.Do(&msflag);
    state.Do(&smflag);
    state.Do(&control);
    state.Do(&SIF0_FIFO);
    state.Do(&SIF1_FIFO);
}

void VectorInterface::do_state(StateSerializer& state)
{
    /*
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
        */

    state.Do(&FIFO);
    state.Do(&internal_FIFO);

    state.Do(&imm);
    state.Do(&command);
    state.Do(&mpg);
    state.Do(&unpack);
    state.Do(&wait_for_VU);
    state.Do(&flush_stall);
    state.Do(&wait_cmd_value);
    state.Do(&buffer_size);
    state.DoArray(buffer, 4);

    state.Do(&DBF);
    state.Do(&CYCLE);
    state.Do(&OFST);
    state.Do(&BASE);
    state.Do(&TOP);
    state.Do(&TOPS);
    state.Do(&ITOP);
    state.Do(&ITOPS);
    state.Do(&MODE);
    state.Do(&MASK);
    state.DoArray(ROW, 4);
    state.DoArray(COL, 4);
    state.Do(&CODE);
    state.Do(&command_len);

    state.Do(&vif_ibit_detected);
    state.Do(&vif_interrupt);
    state.Do(&vif_stalled);
    state.Do(&vif_stop);
    state.Do(&vif_forcebreak);
    state.Do(&vif_cmd_status);
    state.Do(&internal_WL);

    state.Do(&mark_detected);
    state.Do(&VIF_ERR);
}

/*
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
*/

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
    state.DoBytes(read_buffer, 4096);
    state.Do(&ISTAT);
    state.Do(&drive_status);
    state.Do(&is_spinning);

    state.Do(&active_N_command);
    state.Do(&N_command);
    state.DoArray(N_command_params, 11);
    state.Do(&N_params);
    state.Do(&N_status);

    state.Do(&S_command);
    state.DoArray(S_command_params, 16);
    state.DoArray(S_outdata, 16);
    state.Do(&S_params);
    state.Do(&S_out_params);
    state.Do(&S_status);
    state.Do(&rtc);
}

void Scheduler::do_state(StateSerializer& state)
{
    state.Do(&ee_cycles);
    state.Do(&bus_cycles);
    state.Do(&iop_cycles);
    state.Do(&run_cycles);
    state.Do(&closest_event_time);

    state.Do(&events);
    state.Do(&timers);
    /*
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
    */
}

/*
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
*/

void Gamepad::do_state(StateSerializer& state)
{
    state.DoArray(command_buffer, 25);
    state.DoArray(rumble_values, 8);
    state.Do(&mode_lock);
    state.Do(&command);
    state.Do(&command_length);
    state.Do(&data_count);
    state.Do(&pad_mode);
    state.Do(&config_mode);
}

void SPU::do_state(StateSerializer& state)
{
    state.DoArray(voices, 24);
    state.DoArray(core_att, 2);
    state.Do(&status);
    state.Do(&spdif_irq);
    state.Do(&transfer_addr);
    state.Do(&current_addr);
    state.Do(&autodma_ctrl);
    state.Do(&buffer_pos);
    state.DoArray(IRQA, 2);
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
