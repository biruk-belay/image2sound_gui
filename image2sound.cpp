#include "image2sound.h"
#include "ui_image2sound.h"
#include "extract_rgb.h"
#include "rgb_to_midi.h"
#include "midi_files.h"
#include "composer.h"

#include <QFileDialog>
#include <QDebug>
#include <iostream>


image2sound::image2sound(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::image2sound)
{
    ui->setupUi(this);

    init_mux();
    init_cond();

    image2sound::trig_pts = {50,200, 40,0};

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
    int frame_width, frame_height;
    int status;
    pthread_attr_t *attr_ptr = &attr[EXTRACT_RGB];

    QString local_image_path; //stores a copy of previous image path
    local_image_path = imagePath; //save previous path to check if new image is loaded
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

    ui->graphicsView->fitInView(QRectF(0, 0, frame_width, frame_height), Qt::KeepAspectRatio);
    ui->graphicsView->setScene(scene);
}

void image2sound::play_button_pressed()
{
    int status, i;
    int thread_number[] = {THREAD1, THREAD2, THREAD3, THREAD4, COMPOSER_th_1,
                           COMPOSER_th_2, COMPOSER_th_3, COMPOSER_th_4, ALSA_THREAD, RGB_TO_MIDI};
    int thread_priority[] = {20, 20, 20, 20, 30, 20, 20, 20, 20, 20};
    int thread_period[]   = {250, 250, 250, 250, 200, 250, 250, 250, 250, 50};

    qDebug() << "PLAY BUTTON PRESSED: img_size_global" << img_size_global.height << endl;

     status = open_seq(&seq_handler, &alsa_input_port, alsa_output_port);
     if(status != 0)
         qDebug() << "error creating sequencer" << endl;

    for(i = 0; i < TOTAL_THREADS - 1; i++) {

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
                midi_addr = {seq_handler, alsa_output_port[0]};
                break;
            case COMPOSER_th_2 :
                midi_addr = {seq_handler, alsa_output_port[1]};
                break;
            case COMPOSER_th_3 :
                midi_addr = {seq_handler, alsa_output_port[2]};
                break;
            case COMPOSER_th_4 :
                midi_addr = {seq_handler, alsa_output_port[3]};
                break;
            case ALSA_THREAD :
                midi_addr = {seq_handler, alsa_output_port[4]};
                break;
            }

            th_args[thread_number[i]].extra_params = (void *) &midi_addr;
        }

        pthread_attr_init(&attr[thread_number[i]]);
        pthread_attr_setschedpolicy(&attr[thread_number[i]], SCHED_FIFO);\
        sch_par.__sched_priority = tp[thread_number[i]].priority;
        pthread_attr_setschedparam(&attr[thread_number[i]], &sch_par);

    }

    //Add a way to safely manage multiple presses of the play button
    status = pthread_create(&threads[RGB_TO_MIDI], &attr[RGB_TO_MIDI],
                            rgb_to_midi, (void *) tp[RGB_TO_MIDI].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;
    status = pthread_create(&threads[THREAD1], &attr[THREAD1],
                            func, (void *) tp[THREAD1].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    status = pthread_create(&threads[THREAD2], &attr[THREAD2],
                            func, (void *) tp[THREAD2].arg);



    status = pthread_create(&threads[COMPOSER_th_1], &attr[COMPOSER_th_1],
                           composer, (void *) tp[COMPOSER_th_1].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    status = pthread_create(&threads[COMPOSER_th_2], &attr[COMPOSER_th_2],
                           composer, (void *) tp[COMPOSER_th_2].arg);

    //status = pthread_create(&threads[COMPOSER_th_3], &attr[COMPOSER_th_3],
      //                     composer, (void *) tp[COMPOSER_th_3].arg);

    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    status = pthread_create(&threads[ALSA_THREAD], &attr[ALSA_THREAD],
                            alsa_handler, (void *) tp[ALSA_THREAD].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

}

void image2sound::stop_button_pressed()
{

}
long wcet;
int period;
int deadline;  //relative deadline
int priority;
struct timespec activation_t;
struct timespec dead_line;


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

/* Open ALSA sequencer wit num_in writeable ports and num_out readable ports. */
/* The sequencer handle and the port IDs are returned.                        */
int image2sound::open_seq(snd_seq_t **seq_handle, int *in_ports, int *out_ports)
{

    int i;
    char portname[64];

    if (snd_seq_open(seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        qDebug() << " error opening sequencer" << endl;
        return(-1);
    }

    snd_seq_set_client_name(*seq_handle, "MIDI Router");

    for (i = 0; i < ALSA_INPUT_PORTS; i++) {
        sprintf(portname, "MIDI Router IN %d", i);
        if ((in_ports[i] = snd_seq_create_simple_port(*seq_handle, portname,
              SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
              SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
            qDebug() << "error creating ports on sequencer" << endl;
            return(-1);
        }
    }

    for (i = 0; i < ALSA_OUTPUT_PORTS; i++) {
        sprintf(portname, "MIDI Router IN %d", i);
        if ((out_ports[i] = snd_seq_create_simple_port(*seq_handle, portname,
              SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
              SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
            qDebug() << "error creating ports on sequencer" << endl;
            return(-1);
        }
    }
    return(0);
}
