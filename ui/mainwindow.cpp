
#include "mainwindow.h"

MainWindow::MainWindow() : Fl_Window(800, 600)
{
	m_box = std::unique_ptr<Fl_Box>(new Fl_Box(10, 10, 780, 580, "Goldmine-Quik Gateway v2"));
	m_box->box(FL_UP_BOX);
	m_box->labelfont(FL_BOLD);
	this->end();
}

MainWindow::~MainWindow()
{
}

