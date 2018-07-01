#include "image2sound.h"
#include "ui_image2sound.h"
#include "extract_rgb.h"
#include "rgb_to_midi.h"

#include <QFileDialog>
#include <QDebug>
#include <iostream>


image2sound::image2sound(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::image2sound)
{
    ui->setupUi(this);
    init_task_param();
    trig_pts = {50,0,0,0};

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
    image_info img_info;
    pthread_attr_t *attr_ptr = &attr[EXTRACT_RGB];

    QString local_image_path; //stores a copy of previous image path
    local_image_path = imagePath; //save previous path to check if new image is loaded
    imagePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("JPEG (*.jpg *.jpeg);;PNG (*.png)" ));

    qDebug() << "LOAD PRESSED: image path is " << imagePath << endl;
    img_info.filename = &imagePath;
    img_info.img_size = &img_size_global;

    //fill task parameters
    fill_task_param(EXTRACT_RGB, &img_info, 0, 0, 0, 20);

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
    int status;
    thread_arg t_arg;
    thread_arg th_args[NUM_THREADS];
    pthread_attr_t *attr_ptr = &attr[RGB_TO_MIDI];

    t_arg.task_parameter = (void *) &tp[RGB_TO_MIDI];
    t_arg.img_size = &img_size_global;

    qDebug() << "PLAY BUTTON PRESSED: img_size_global" << img_size_global.height << endl;

    fill_task_param(RGB_TO_MIDI, (void*) &tp[RGB_TO_MIDI], 0, 10, 0, 20);
    pthread_attr_init(attr_ptr);
    pthread_attr_setschedpolicy(attr_ptr, SCHED_FIFO);
    sch_par.__sched_priority = tp[EXTRACT_RGB].priority;
    pthread_attr_setschedparam(attr_ptr, &sch_par);

    //Add a way to safely manage multiple presses of the play button
    status = pthread_create(&threads[RGB_TO_MIDI], attr_ptr,
                            rgb_to_midi, (void *) &t_arg);
    if(status != 0)
        qDebug() << "PLAY BUTTON PRESSED: thread not created" << status << endl;

    //Initialize the attributes and task parameters for the remaining threads
    th_args[THREAD_1].img_size = &img_size_global;
    th_args[THREAD_1].task_parameter = (void *) &comp_buff[THREAD_1];

    fill_task_param(THREAD_1 + 2, (void*) &tp[RGB_TO_MIDI], 0, 10, 0, 20);


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


void image2sound::init_task_param()
{
    //fill_task_param(EXTRACT_RGB, 0, 0, 0, 20);
  //  qDebug() << tp[EXTRACT_RGB].priority << endl;

}

void image2sound::copy_to_vector(unsigned char *buffer, unsigned int size)
{
     image2sound::rgb_vector.insert(rgb_vector.begin(), buffer, buffer + size);
}
