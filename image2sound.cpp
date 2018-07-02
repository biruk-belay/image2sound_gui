#include "image2sound.h"
#include "ui_image2sound.h"
#include "extract_rgb.h"
#include "rgb_to_midi.h"
#include "midi_files.h"

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

    image2sound::trig_pts = {50,0,0,0};

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

    //img_info.filename = &imagePath;
    //img_info.img_size = &img_size_global;

    //fill task parameters
    fill_task_param(EXTRACT_RGB, EXTRACT_RGB, (void *) &th_args[EXTRACT_RGB], 0, 0, 0, 20);

    //setting attributes of thread which handles the extarction of rgb
    pthread_attr_init(attr_ptr);
    //pthread_attr_setinheritsched(attr_ptr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(attr_ptr, SCHED_FIFO);

    sch_par.__sched_priority = tp[EXTRACT_RGB].priority;
    pthread_attr_setschedparam(attr_ptr, &sch_par);
    //pthread_attr_setstack(&attr[EXTRACT_RGB], &stack, 65536000);
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
    int thread_number[] = {THREAD1, THREAD2, THREAD3, THREAD4, RGB_TO_MIDI};

    qDebug() << "PLAY BUTTON PRESSED: img_size_global" << img_size_global.height << endl;

    for(i = 0; i < 5; i++) {

        fill_task_param(thread_number[i], thread_number[i], (void*) &th_args[thread_number[i]], 0, 10, 0, 20);

        th_args[thread_number[i]].task_parameter = &tp[thread_number[i]];
        th_args[thread_number[i]].img_size = &img_size_global;
        if(thread_number[i] == THREAD1 || thread_number[i] == THREAD2 ||
                thread_number[i] == THREAD3 || thread_number[i] == THREAD4 )
            th_args[thread_number[i]].extra_params = (void *) &comp_buff[thread_number[i]];

        pthread_attr_init(&attr[thread_number[i]]);
        pthread_attr_setschedpolicy(&attr[thread_number[i]], SCHED_FIFO);\
        sch_par.__sched_priority = tp[thread_number[i]].priority;
        pthread_attr_setschedparam(&attr[thread_number[i]], &sch_par);

    }
    //Fill the task parameters for the thread which converts RGB to MIDI
/*
    th_args[RGB_TO_MIDI].task_parameter = &tp[RGB_TO_MIDI];
    th_args[RGB_TO_MIDI].img_size = &img_size_global;

    fill_task_param(RGB_TO_MIDI, RGB_TO_MIDI, (void*) &th_args[RGB_TO_MIDI], 0, 10, 0, 20);

    pthread_attr_init(&attr[RGB_TO_MIDI]);
    pthread_attr_setschedpolicy(&attr[RGB_TO_MIDI], SCHED_FIFO);
    sch_par.__sched_priority = tp[RGB_TO_MIDI].priority;
    pthread_attr_setschedparam(&attr[RGB_TO_MIDI], &sch_par);

    //Add a way to safely manage multiple presses of the play button
  */
    status = pthread_create(&threads[RGB_TO_MIDI], &attr[RGB_TO_MIDI],
                            rgb_to_midi, (void *) tp[RGB_TO_MIDI].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    /*Initialize attributes & fill the task parameters for the threads which
     * interact with the MIDI thread
    */
    //for loop here later
    /*th_args[first_thread].img_size = &img_size_global;
    //array of structure which contains vectors to buffer MIDI values for the MIDI thread
    th_args[first_thread].extra_params = (void *) &comp_buff[first_thread];
    th_args[first_thread].task_parameter = &tp[THREAD1];

    fill_task_param(THREAD1, first_thread, (void *) &th_args[first_thread], 0, 25, 0, 18);
    //Initialize the attributes and task parameters for the remaining threads
    pthread_attr_init(&attr[THREAD1]);
    pthread_attr_setschedpolicy(&attr[THREAD1], SCHED_FIFO);
    sch_par.__sched_priority = tp[THREAD1].priority;
    pthread_attr_setschedparam(&attr[THREAD1], &sch_par);
*/
    status = pthread_create(&threads[THREAD1], &attr[THREAD1],
                            func, (void *) tp[THREAD1].arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    qDebug() << "period in main" << tp[RGB_TO_MIDI].period << endl;
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
