#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <math.h>
#include <stdlib.h>
#include <qmmp/buffer.h>
#include <qmmp/output.h>
#include "fft.h"
#include "inlines.h"
#include "ripples.h"

#define VISUAL_NODE_SIZE 512 //samples
#define VISUAL_BUFFER_SIZE (5*VISUAL_NODE_SIZE)

Ripples::Ripples (QWidget *parent) : Visual (parent)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_intern_vis_data = 0;
    m_x_scale = 0;
    m_buffer_at = 0;
    m_rows = 0;
    m_cols = 0;
    m_analyzer_falloff = 2.2;
    m_cell_size = QSize(15, 6);

    setWindowTitle (tr("Ripples"));
    m_timer = new QTimer (this);
    m_timer->setInterval(40);
    connect(m_timer, SIGNAL (timeout()), this, SLOT (timeout()));
    m_buffer = new float[VISUAL_BUFFER_SIZE];

    clear();
}

Ripples::~Ripples()
{
    delete [] m_buffer;

    if(m_intern_vis_data)
        delete [] m_intern_vis_data;
    if(m_x_scale)
        delete [] m_x_scale;
}

void Ripples::add (float *data, size_t samples, int chan)
{
    if (!m_timer->isActive ())
        return;

    if(VISUAL_BUFFER_SIZE == m_buffer_at)
    {
        m_buffer_at -= VISUAL_NODE_SIZE;
        memmove(m_buffer, m_buffer + VISUAL_NODE_SIZE, m_buffer_at * sizeof(float));
        return;
    }

    int frames = qMin(int(samples/chan), VISUAL_BUFFER_SIZE - m_buffer_at);

    mono_from_multichannel(m_buffer + m_buffer_at, data, frames, chan);

    m_buffer_at += frames;
}

void Ripples::clear()
{
    m_buffer_at = 0;
    m_rows = 0;
    m_cols = 0;
    update();
}

void Ripples::timeout()
{
    mutex()->lock();
    if(m_buffer_at < VISUAL_NODE_SIZE)
    {
        mutex()->unlock ();
        return;
    }

    process (m_buffer);
    m_buffer_at -= VISUAL_NODE_SIZE;
    memmove(m_buffer, m_buffer + VISUAL_NODE_SIZE, m_buffer_at * sizeof(float));
    mutex()->unlock ();
    update();
}

void Ripples::hideEvent (QHideEvent *)
{
    m_timer->stop();
}

void Ripples::showEvent (QShowEvent *)
{
    m_timer->start();
}

void Ripples::paintEvent (QPaintEvent * e)
{
    Q_UNUSED(e);
    QPainter painter (this);
    draw(&painter);
}

void Ripples::process (float *buffer)
{
    int rows = (height() - 2) / m_cell_size.height();
    int cols = (width() - 2) / m_cell_size.width();

    if(m_rows != rows || m_cols != cols)
    {
        m_rows = rows;
        m_cols = cols;
        if(m_intern_vis_data)
            delete [] m_intern_vis_data;
        if(m_x_scale)
            delete [] m_x_scale;
        m_intern_vis_data = new double[m_cols];
        m_x_scale = new int[m_cols + 1];

        for(int i = 0; i < m_cols; ++i)
        {
            m_intern_vis_data[i] = 0;
        }
        for(int i = 0; i < m_cols + 1; ++i)
            m_x_scale[i] = pow(pow(255.0, 1.0 / m_cols), i);
    }

    short dest[256];
    short y;
    int k, magnitude;

    calc_freq (dest, buffer);

    double y_scale = (double) 1.25 * m_rows / log(256);

    for (int i = 0; i < m_cols; i++)
    {
        y = 0;
        magnitude = 0;

        if(m_x_scale[i] == m_x_scale[i + 1])
        {
            y = dest[i];
        }
        for (k = m_x_scale[i]; k < m_x_scale[i + 1]; k++)
        {
            y = qMax(dest[k], y);
        }

        y >>= 7; //256

        if (y)
        {
            magnitude = int(log (y) * y_scale);
            magnitude = qBound(0, magnitude, m_rows);
        }

        m_intern_vis_data[i] -= m_analyzer_falloff * m_rows / 15;
        m_intern_vis_data[i] = magnitude > m_intern_vis_data[i] ? magnitude : m_intern_vis_data[i];
    }
}

void Ripples::draw (QPainter *p)
{
    QBrush brush(Qt::SolidPattern);
    int x = 0;
    int rdx = qMax(0, width() - 2 * m_cell_size.width() * m_cols);

    for (int j = 0; j < m_cols; ++j)
    {
        x = j * m_cell_size.width() + 1;
        if(j >= m_cols)
            x += rdx; //correct right part position

        for (int i = 0; i <= m_intern_vis_data[j]; ++i)
        {
            brush.setColor(Qt::white);
            p->fillRect (x, height() - i * m_cell_size.height() + 1,
                         m_cell_size.width() - 2, m_cell_size.height() - 2, brush);
        }
    }
}
