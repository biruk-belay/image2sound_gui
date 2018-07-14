#include "image2sound.h"
#include "ui_image2sound.h"
#include "extract_rgb.h"
#include "rgb_to_midi.h"
#include "midi_files.h"
#include "composer.h"

#include <QFileDialog>
#include <QDebug>
#include <iostream>

#define T_QUARTER           150
#define T_8th T_QUARTER >>  1
#define T_16th T_QUARTER >> 2
#define T_min T_QUARTER >>  3

image2sound::image2sound(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::image2sound)
{
    ui->setupUi(this);

    mTimer = new QTimer(this);

    init_mux();
    init_cond();
    init_task_state();

        //int quarter[] = {1, 0, 1, 1, 0, 1, 0};
    //rhytm1.quarter = quarter;

    connect(mTimer, SIGNAL(timeout()), this, SLOT(update_scene()));
    connect(ui->Load_button, SIGNAL(released()), this, SLOT(load_button_pressed()));
    connect(ui->Play_button, SIGNAL(released()), this, SLOT(play_button_pressed()));
    connect(ui->Stop_button, SIGNAL(released()), this, SLOT(stop_button_pressed()));
 }

image2sound::~image2sound()
{
    delete ui;
}

void image2sound::load_button_pressed()
{
    int status, i;
    double scale_factor, next_column;
    pthread_attr_t *attr_ptr = &attr[EXTRACT_RGB];
    Qt::GlobalColor colors [] = {Qt::GlobalColor::red,   Qt::GlobalColor::yellow,
                      Qt::GlobalColor::blue, Qt::GlobalColor::magenta, Qt::GlobalColor::cyan};
    int thread_number[] =  {THREAD1, THREAD2, THREAD3, THREAD4, RGB_TO_MIDI};

    image2sound::stop_button_pressed();

    QString local_image_path; //stores a copy of previous image path
    local_image_path = image2sound::imagePath; //save previous path to check if new image is loaded
    imagePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));

    qDebug() << "LOAD PRESSED: image path is " << imagePath << endl;
    th_args[EXTRACT_RGB].task_parameter = &tp[EXTRACT_RGB];
    th_args[EXTRACT_RGB].img_size = &img_size_global;
    th_args[EXTRACT_RGB].extra_params = (void *) &imagePath;

    //fill task parameters
    fill_task_param(EXTRACT_RGB, EXTRACT_RGB, (void *) &th_args[EXTRACT_RGB], 0, 0, 0, 20);

    //setting attributes of thread which handles the extarction of rgb
    pthread_attr_init(attr_ptr);
    //pthread_attr_setinheritsched(attr_ptr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(attr_ptr, SCHED_FIFO);

    sch_par.__sched_priority = tp[EXTRACT_RGB].priority;
    pthread_attr_setschedparam(attr_ptr, &sch_par);

    /*if image path fails to load (i.e. the image is not updated) restore the previous path
     *  else process the newly loaded image with a thread */
    if(imagePath == "")
        imagePath = local_image_path;
    else {
      //flush the midi and the rgb buffers before calling the thread
        image2sound::rgb_vector.clear();
        image2sound::rgb_vector.swap(rgb_vector);

        image2sound::midi_vector.clear();
        image2sound::midi_vector.swap(midi_vector);

        for(i = 0; i < NUM_THREADS; i++) {
            comp_buff[i].buffer.clear();
            comp_buff[i].buffer.swap(comp_buff[i].buffer);
        }

        status = pthread_create(&threads[EXTRACT_RGB], attr_ptr,
                       extract_rgb, (void *) tp[EXTRACT_RGB].arg);
       if(status != 0)
           qDebug() << "error creating thread" << status << endl;
    }
    image.load(imagePath);

    //get width and height of the greaphicsview to resize the image
    frame_width = ui->graphicsView->geometry().width();
    frame_height = ui->graphicsView->geometry().height();

    //remove vertical and horizontal scroll
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene = new QGraphicsScene(QRectF(0, 0, frame_width, frame_height), 0);
    scene->addPixmap(image.scaled(QSize((int)scene->width(), (int)scene->height())));


    scale_factor = ((double) (img_size_global.width ))/ ((double) image2sound::frame_width);
    scene->addPixmap(image.scaled(QSize((int)scene->width(), (int)scene->height())));

    for(i = 0; i < NUM_THREADS + 1; i++)
        if(!tsk_state[thread_number[i]].is_active)
            scene->addRect(trig_pts[i].x /scale_factor , trig_pts[i].y,
                           2, 2, QPen(QBrush(colors[i]), 2));

    ui->graphicsView->fitInView(QRectF(0, 0, frame_width, frame_height), Qt::KeepAspectRatio);
    ui->graphicsView->setScene(scene);
}

void image2sound::play_button_pressed()
{
    int status, i;
    int thread_number[] =  {THREAD1, THREAD2, THREAD3, THREAD4,
                         COMPOSER_th_1, COMPOSER_th_2,
                         COMPOSER_th_3, COMPOSER_th_4,
                         ALSA_THREAD, RGB_TO_MIDI, MONITOR_THREAD,
                         UPDATE_SCENE};

    int thread_priority[] = {30, 30, 30, 30, 20, 20, 20, 20, 20, 20, 20, 20};
    int thread_period[]   = {T_QUARTER, T_8th, T_16th, T_8th,
                             T_QUARTER , T_8th, T_16th, T_8th,
                             T_min, T_min, 1000, 1000};

    qDebug() << "PLAY BUTTON PRESSED: img_size_global" << img_size_global.height << endl;

    //start timer to update graphics
    mTimer->start(100);

    if(!tsk_state[ALSA_THREAD].is_active){
        status = open_seq(&seq_handler, &alsa_input_port, alsa_output_port);

        if(status != 0)
            qDebug() << "error creating sequencer" << endl;
    }

    for(i = 0; i < sizeof(thread_number) / sizeof(int); i++) {

        fill_task_param(thread_number[i], thread_number[i],
                        (void*) &th_args[thread_number[i]], 0,
                       thread_period[i], 0, thread_priority[i]);

        th_args[thread_number[i]].task_parameter = &tp[thread_number[i]];
        th_args[thread_number[i]].img_size = &img_size_global;

        if(thread_number[i] == THREAD1 || thread_number[i] == THREAD2 ||
                thread_number[i] == THREAD3 || thread_number[i] == THREAD4)
            th_args[thread_number[i]].extra_params = (void *) &comp_buff[thread_number[i]];

        if(thread_number[i] == COMPOSER_th_1 || thread_number[i] == COMPOSER_th_2 ||
                thread_number[i] == COMPOSER_th_3 || thread_number[i] == COMPOSER_th_4 ||
                thread_number[i] == ALSA_THREAD) {

            switch (thread_number[i]) {

            case COMPOSER_th_1 :
                generate_rhythm(rhytm1[0].quarter, QUARTER, BASS);
                midi_addr[0] = {seq_handler, alsa_output_port[0], CHAN_1};
                synth_data[0].midi_addr = &midi_addr[0];
                synth_data[0].sequence  = rhytm1[0].quarter;
                synth_data[0].type = QUARTER;
                synth_data[0].instr = BASS;
                th_args[thread_number[i]].extra_params = (void *) &synth_data[0];
                break;

            case COMPOSER_th_2 :
                generate_rhythm(rhytm1[1].eigth, EIGTH, PIANO);
                midi_addr[1] = {seq_handler, alsa_output_port[1], CHAN_2};
                synth_data[1].midi_addr = &midi_addr[1];
                synth_data[1].sequence  = rhytm1[1].eigth;
                synth_data[1].type = EIGTH;
                synth_data[1].instr = PIANO;
                th_args[thread_number[i]].extra_params = (void *) &synth_data[1];
                break;

            case COMPOSER_th_3 :
                generate_rhythm(rhytm1[2].sixtinth, SIXTINTH, GUITAR);
                midi_addr[2] = {seq_handler, alsa_output_port[2], CHAN_3};
                synth_data[2].midi_addr = &midi_addr[2];
                synth_data[2].sequence = rhytm1[2].sixtinth;
                synth_data[2].type =SIXTINTH;
                synth_data[2].instr = GUITAR;
                th_args[thread_number[i]].extra_params = (void *) &synth_data[2];
                break;

            case COMPOSER_th_4 :
                midi_addr[3] = {seq_handler, alsa_output_port[3]};
                th_args[thread_number[i]].extra_params = (void *) &midi_addr[3];
                break;
            case ALSA_THREAD :
                midi_addr[4] = {seq_handler, alsa_output_port[4]};
                th_args[thread_number[i]].extra_params = (void *) &midi_addr[4];
                break;
            }
        }

        pthread_attr_init(&attr[thread_number[i]]);
        pthread_attr_setschedpolicy(&attr[thread_number[i]], SCHED_FIFO);\
        sch_par.__sched_priority = tp[thread_number[i]].priority;
        pthread_attr_setschedparam(&attr[thread_number[i]], &sch_par);

    }

    //Add a way to safely manage multiple presses of the play button
    if(!tsk_state[RGB_TO_MIDI].is_active) {
        status = pthread_create(&threads[RGB_TO_MIDI], &attr[RGB_TO_MIDI],
                            rgb_to_midi, (void *) tp[RGB_TO_MIDI].arg);
        if(status != 0)
            qDebug() << "PLAY BUTTON PRESSED: rgb_to_midi thread not created" << status << endl;
    }

    if(!tsk_state[THREAD1].is_active) {
        status = pthread_create(&threads[THREAD1], &attr[THREAD1],
                               func, (void *) tp[THREAD1].arg);
        if(status != 0)
            qDebug() << "PLAY BUTTON PRESSED: copy_to buff thread not created" << status << endl;
    }

    if(!tsk_state[COMPOSER_th_1].is_active) {
        status = pthread_create(&threads[COMPOSER_th_1], &attr[COMPOSER_th_1],
                           synthesizer, (void *) tp[COMPOSER_th_1].arg);
        if(status != 0)
            qDebug() << "PLAY BUTTON PRESSED: synthesizer thread not created" << status << endl;
    }


    if(!tsk_state[THREAD2].is_active) {
         status = pthread_create(&threads[THREAD2], &attr[THREAD2],
                             func, (void *) tp[THREAD2].arg);
         if(status != 0)
                qDebug() << "PLAY BUTTON PRESSED: copy_to_buff thread not created" << status << endl;
        }

    if(!tsk_state[COMPOSER_th_2].is_active) {
        status = pthread_create(&threads[COMPOSER_th_2], &attr[COMPOSER_th_2],
                           synthesizer, (void *) tp[COMPOSER_th_2].arg);

        if(status != 0)
            qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    }

    if(!tsk_state[THREAD3].is_active) {
         status = pthread_create(&threads[THREAD3], &attr[THREAD3],
                             func, (void *) tp[THREAD3].arg);
         if(status != 0)
                qDebug() << "PLAY BUTTON PRESSED: copy_to_buff thread not created" << status << endl;
        }

    if(!tsk_state[COMPOSER_th_3].is_active) {
        status = pthread_create(&threads[COMPOSER_th_3], &attr[COMPOSER_th_3],
                           synthesizer, (void *) tp[COMPOSER_th_3].arg);

        if(status != 0)
            qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    }

    if(!tsk_state[ALSA_THREAD].is_active) {
        status = pthread_create(&threads[ALSA_THREAD], &attr[ALSA_THREAD],
                            alsa_handler, (void *) tp[ALSA_THREAD].arg);
        if(status != 0)
            qDebug() << "PLAY BUTTON PRESSED: alsa thread not created" << status << endl;
    }

    status = pthread_create(&threads[MONITOR_THREAD], &attr[MONITOR_THREAD],
                            monitor, (void *) tp[MONITOR_THREAD].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: monitor thread not created" << status << endl;


}

void image2sound::stop_button_pressed()
{
    int i;
    int status;

    int thread_number[] =  {THREAD1, THREAD2, THREAD3, THREAD4,
                           COMPOSER_th_1, COMPOSER_th_2,
                           COMPOSER_th_3, COMPOSER_th_4,
                           RGB_TO_MIDI};

    for(i = 0; i < sizeof(thread_number) / sizeof(int); i++) {
        if(tsk_state[thread_number[i]].is_active) {
            cancel_th[thread_number[i]].kill_thread = true;
            qDebug() << "STOP BUTTON: thread status " << status << thread_number[i] << endl;

        }
    }

    //Reset the trigger states
    for(i = 0; i < sizeof(is_triggered) / sizeof(bool); i++)
        is_triggered[i] = false;

    //also stop the timer which updates the rectangles on the gui
    //mTimer->stop();
}



void image2sound::copy_to_vector(unsigned char *buffer, unsigned int size)
{
     image2sound::rgb_vector.insert(rgb_vector.begin(), buffer, buffer + size);
}

void image2sound::init_mux()
{
    int i;
    for(i = 0; i < NUM_THREADS; i++)
        pthread_mutex_init(&image2sound::sync_mutex[i], NULL);
}

void image2sound::init_cond()
{
    int i;
    for(i = 0; i < NUM_THREADS; i++)
        pthread_cond_init(&image2sound::sync_cond[i], NULL);
}

int image2sound::open_seq(snd_seq_t **seq_handle, int *in_ports, int *out_ports)
{
    int i;
    char portname[64];

    //open alsa sequencer
    if (snd_seq_open(seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        qDebug() << " error opening sequencer" << endl;
        return(-1);
    }

    assert(seq_handle != NULL);

    //There are 5 output ports and 1 input port
    snd_seq_set_client_name(*seq_handle, "Image2Sound");

    for (i = 0; i < ALSA_INPUT_PORTS; i++) {
        sprintf(portname, "ALSA router input");
        if ((in_ports[i] = snd_seq_create_simple_port(*seq_handle, portname,
              SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
              SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
            qDebug() << "error creating ports on sequencer" << endl;
            return(-1);
        }
    }

    for (i = 0; i < ALSA_OUTPUT_PORTS; i++) {
        if(i != ALSA_OUTPUT_PORTS - 1)
            sprintf(portname, "Channel %d", i);
        else
            sprintf(portname, "ALSA router output");
        if ((out_ports[i] = snd_seq_create_simple_port(*seq_handle, portname,
              SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
              SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
            qDebug() << "error creating ports on sequencer" << endl;
            return(-1);
        }
    }
    return(0);
}

//initialize task state
void image2sound::init_task_state()
{
    int i;

    for(i = 0; i < sizeof(tsk_state)/sizeof(task_state *); i++) {
        tsk_state[i].is_active = false;
        tsk_state[i].missed_deadline = 0;
    }
}

void image2sound::update_task_state(int task_id)
{
    tsk_state[task_id].is_active = true;
}

void image2sound::update_missed_deadline(int task_id)
{
    tsk_state[task_id].missed_deadline += 1;
}

void image2sound::update_scene()
{
    int i;
    double scale_factor, next_column ;
    int thread_number[] =  {THREAD1, THREAD2, THREAD3, THREAD4, RGB_TO_MIDI};
    Qt::GlobalColor colors [] = {Qt::GlobalColor::red, Qt::GlobalColor::yellow,
                      Qt::GlobalColor::blue, Qt::GlobalColor::magenta, Qt::GlobalColor::cyan};

    scale_factor = ((double) (img_size_global.width ))/ ((double) image2sound::frame_width);

    //Redreaw image
    scene->addPixmap(image.scaled(QSize((int)scene->width(), (int)scene->height())));

    //update the rectangle positions
    for(i = 0; i < NUM_THREADS + 1; i++) {
        if(!tsk_state[thread_number[i]].is_active)
            scene->addRect(trig_pts[i].x / scale_factor, trig_pts[i].y,
                           2, 2, QPen(QBrush(colors[i]), 2));
        else {
            next_column = ((double) rect[i].x) / scale_factor;
            scene->addRect(next_column, rect[i].y,
                   2, 2, QPen(QBrush(colors[i]), 2));
        }
    }
}

void image2sound::generate_rhythm(int *rhythm, enum tempo type, enum instrument instr)
{
    int i;
    int count = 0;
    int array [type];

//    if(rhythm_type == BASELINE) {
        do {

            for(i = 0; i < type; i++) {
                array[i] = rand() % 2;
                if(array[i] == 1)
                    count++;
            }
        } while(instr == PIANO ? count < type/2 : count < type/3);

        for(i = 0; i < type; i++) {
            *(rhythm + i) = array[i];
            qDebug() << "rhythm is" << *(rhythm + i) << i;
        }
}
