// taken from iris

#include <qtimer.h>

#include "client.h"
#include "gwfield.h"
#include "request.h"
#include "safedelete.h"

#include "task.h"

class Task::TaskPrivate
{
public:
	TaskPrivate() {}

	QString id;
	bool success;
	int statusCode;
	QString statusString;
	Client *client;
	bool insignificant, deleteme, autoDelete;
	bool done;
	Transfer * transfer;
};

Task::Task(Task *parent)
:QObject(parent)
{
	init();

	d->client = parent->client();
	d->id = client()->genUniqueId();
	connect(d->client, SIGNAL(disconnected()), SLOT(clientDisconnected()));
}

Task::Task(Client *parent, bool)
:QObject(0)
{
	init();

	d->client = parent;
	connect(d->client, SIGNAL(disconnected()), SLOT(clientDisconnected()));
}

Task::~Task()
{
	delete d;
}

void Task::init()
{
	d = new TaskPrivate;
	d->success = false;
	d->insignificant = false;
	d->deleteme = false;
	d->autoDelete = false;
	d->done = false;
	d->transfer = 0;
}

Task *Task::parent() const
{
	return (Task *)QObject::parent();
}

Client *Task::client() const
{
	return d->client;
}

Transfer * Task::transfer() const
{
	return d->transfer;
}

void Task::setTransfer( Transfer * transfer )
{
	d->transfer = transfer;
}

QString Task::id() const
{
	return d->id;
}

bool Task::success() const
{
	return d->success;
}

int Task::statusCode() const
{
	return d->statusCode;
}

const QString & Task::statusString() const
{
	return d->statusString;
}

void Task::go(bool autoDelete)
{
	d->autoDelete = autoDelete;

	onGo();
}

bool Task::take( const Transfer * transfer)
{
	const QObjectList *p = children();
	if(!p)
		return false;

	// pass along the transfer to our children
	QObjectListIt it(*p);
	Task *t;
	for(; it.current(); ++it) {
		QObject *obj = it.current();
		if(!obj->inherits("Task"))
			continue;

		t = static_cast<Task*>(obj);
		if(t->take( transfer ))
			return true;
	}

	return false;
}

void Task::safeDelete()
{
	if(d->deleteme)
		return;

	d->deleteme = true;
	if(!d->insignificant)
		SafeDelete::deleteSingle(this);
}

void Task::onGo()
{
}

void Task::onDisconnect()
{
	if(!d->done) {
		d->success = false;
		d->statusCode = ErrDisc;
		d->statusString = tr("Disconnected");

		// delay this so that tasks that react don't block the shutdown
		QTimer::singleShot(0, this, SLOT(done()));
	}
}

void Task::send( Request * request )
{
	client()->send( request );
}

void Task::setSuccess(int code, const QString &str)
{
	if(!d->done) {
		d->success = true;
		d->statusCode = code;
		d->statusString = str;
		done();
	}
}

void Task::setError(int code, const QString &str)
{
	if(!d->done) {
		d->success = false;
		d->statusCode = code;
		d->statusString = str;
		done();
	}
}

void Task::done()
{
	if(d->done || d->insignificant)
		return;
	d->done = true;

	if(d->deleteme || d->autoDelete)
		d->deleteme = true;

	d->insignificant = true;
	finished();
	d->insignificant = false;

	if(d->deleteme)
		SafeDelete::deleteSingle(this);
}

void Task::clientDisconnected()
{
	onDisconnect();
}

// void Task::debug(const char *fmt, ...)
// {
// 	char *buf;
// 	QString str;
// 	int size = 1024;
// 	int r;
// 
// 	do {
// 		buf = new char[size];
// 		va_list ap;
// 		va_start(ap, fmt);
// 		r = vsnprintf(buf, size, fmt, ap);
// 		va_end(ap);
// 
// 		if(r != -1)
// 			str = QString(buf);
// 
// 		delete [] buf;
// 
// 		size *= 2;
// 	} while(r == -1);
// 
// 	debug(str);
// }

void Task::debug(const QString &str)
{
	client()->debug(QString("%1: ").arg(className()) + str);
}

bool Task::forMe( const Transfer * transfer ) const
{
	return false;
}

#include "task.moc"
