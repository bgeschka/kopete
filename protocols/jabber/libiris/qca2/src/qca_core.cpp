/*
 * Copyright (C) 2003-2005  Justin Karneges <justin@affinix.com>
 * Copyright (C) 2004,2005  Brad Hards <bradh@frogmouth.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "qca_core.h"

#include <QtCore>
#include "qca_plugin.h"
#include "qca_textfilter.h"
#include "qca_cert.h"
#include "qca_keystore.h"
#include "qcaprovider.h"

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif
namespace QCA {

// from qca_tools
bool botan_init(int prealloc, bool mmap);
void botan_deinit();

// from qca_default
Provider *create_default_provider();

//----------------------------------------------------------------------------
// Global
//----------------------------------------------------------------------------
class Global
{
public:
	bool secmem;
	QString app_name;
	QMutex manager_mutex;
	ProviderManager manager;
	Random *rng;
	KeyStoreManager *ksm;

	Global()
	{
		rng = 0;
		ksm = new KeyStoreManager;
		secmem = false;
	}

	~Global()
	{
		delete ksm;
		delete rng;
	}
};

static Global *global = 0;

static bool features_have(const QStringList &have, const QStringList &want)
{
	for(QStringList::ConstIterator it = want.begin(); it != want.end(); ++it)
	{
		if(!have.contains(*it))
			return false;
	}
	return true;
}

void init()
{
	if(global)
		return;

	init(Practical, 64);
}

void init(MemoryMode mode, int prealloc)
{
	if(global)
		return;

	bool allow_mmap_fallback = false;
	bool drop_root = false;
	if(mode == Practical)
	{
		allow_mmap_fallback = true;
		drop_root = true;
	}
	else if(mode == Locking)
		drop_root = true;

	bool secmem = botan_init(prealloc, allow_mmap_fallback);

	if(drop_root)
	{
#ifdef Q_OS_UNIX
		setuid(getuid());
#endif
	}

	global = new Global;
	global->secmem = secmem;
	global->manager.setDefault(create_default_provider()); // manager owns it

	// for maximum setuid safety, qca should be initialized before qapp:
	//
	//   int main(int argc, char **argv)
	//   {
	//       QCA::Initializer init;
	//       QCoreApplication app(argc, argv);
	//       return 0;
	//   }
	//
	// however, the above code has the unfortunate side-effect of causing
	// qapp to deinit before qca, which can cause problems with any
	// plugins that have active objects (notably KeyStore).  we'll use a
	// post routine to force qca to deinit first.
	qAddPostRoutine(deinit);
}

void deinit()
{
	if(!global)
		return;

	delete global;
	global = 0;
	botan_deinit();
}

bool haveSecureMemory()
{
	if(!global)
		return false;

	return global->secmem;
}

bool isSupported(const QStringList &features, const QString &provider)
{
	if(!global)
		return false;

	QMutexLocker lock(&global->manager_mutex);

	// single
	if(!provider.isEmpty())
	{
		Provider *p = global->manager.find(provider);
		if(!p)
		{
			// ok, try scanning for new stuff
			global->manager.scan();
			p = global->manager.find(provider);
		}

		if(p && features_have(p->features(), features))
			return true;
	}
	// all
	else
	{
		if(features_have(global->manager.allFeatures(), features))
			return true;

		// ok, try scanning for new stuff
		global->manager.scan();

		if(features_have(global->manager.allFeatures(), features))
			return true;
	}
	return false;
}

bool isSupported(const char *features, const QString &provider)
{
	return isSupported(QString(features).split(',', QString::SkipEmptyParts), provider);
}

QStringList supportedFeatures()
{
	init();

	QMutexLocker lock(&global->manager_mutex);

	// query all features
	global->manager.scan();
	return global->manager.allFeatures();
}

QStringList defaultFeatures()
{
	init();

	QMutexLocker lock(&global->manager_mutex);

	return global->manager.find("default")->features();
}

bool insertProvider(Provider *p, int priority)
{
	init();

	QMutexLocker lock(&global->manager_mutex);

	return global->manager.add(p, priority);
}

void setProviderPriority(const QString &name, int priority)
{
	if(!global)
		return;

	QMutexLocker lock(&global->manager_mutex);

	global->manager.changePriority(name, priority);
}

int providerPriority(const QString &name)
{
	if(!global)
		return -1;

	QMutexLocker lock(&global->manager_mutex);

	return global->manager.getPriority(name);
}

ProviderList providers()
{
	init();

	QMutexLocker lock(&global->manager_mutex);

	return global->manager.providers();
}

Provider *findProvider(const QString &name)
{
	QMutexLocker lock(&global->manager_mutex);

	return global->manager.find(name);
}

Provider *defaultProvider()
{
	QMutexLocker lock(&global->manager_mutex);

	return global->manager.find("default");
}

void scanForPlugins()
{
	QMutexLocker lock(&global->manager_mutex);

	global->manager.scan();
}

void unloadAllPlugins()
{
	if(!global)
		return;

	QMutexLocker lock(&global->manager_mutex);

	// if the global_rng was owned by a plugin, then delete it
	if(global->rng && (global->rng->provider() != global->manager.find("default")))
	{
		delete global->rng;
		global->rng = 0;
	}

	global->manager.unloadAll();
}

void setProperty(const QString &name, const QVariant &value)
{
	QMutexLocker lock(&global->manager_mutex);

	Q_UNUSED(name);
	Q_UNUSED(value);
}

QVariant getProperty(const QString &name)
{
	QMutexLocker lock(&global->manager_mutex);

	Q_UNUSED(name);
	return QVariant();
}

Random & globalRNG()
{
	if(!global->rng)
		global->rng = new Random;
	return *global->rng;
}

void setGlobalRNG(const QString &provider)
{
	delete global->rng;
	global->rng = new Random(provider);
}

KeyStoreManager *keyStoreManager()
{
	return global->ksm;
}

bool haveSystemStore()
{
	// ensure the system store is loaded
	global->ksm->start("default");
	global->ksm->waitForBusyFinished();

	QStringList list = global->ksm->keyStores();
	for(int n = 0; n < list.count(); ++n)
	{
		KeyStore ks(list[n]);
		if(ks.type() == KeyStore::System && ks.holdsTrustedCertificates())
			return true;
	}
	return false;
}

CertificateCollection systemStore()
{
	// ensure the system store is loaded
	global->ksm->start("default");
	global->ksm->waitForBusyFinished();

	CertificateCollection col;
	QStringList list = global->ksm->keyStores();
	for(int n = 0; n < list.count(); ++n)
	{
		KeyStore ks(list[n]);

		// system store
		if(ks.type() == KeyStore::System && ks.holdsTrustedCertificates())
		{
			// extract contents
			QList<KeyStoreEntry> entries = ks.entryList();
			for(int i = 0; i < entries.count(); ++i)
			{
				if(entries[i].type() == KeyStoreEntry::TypeCertificate)
					col.addCertificate(entries[i].certificate());
				else if(entries[i].type() == KeyStoreEntry::TypeCRL)
					col.addCRL(entries[i].crl());
			}
			break;
		}
	}
	return col;
}

QString appName()
{
	if(!global)
		return QString();
	return global->app_name;
}

void setAppName(const QString &s)
{
	if(!global)
		return;
	global->app_name = s;
}

QString arrayToHex(const QSecureArray &a)
{
	return Hex().arrayToString(a);
}

QByteArray hexToArray(const QString &str)
{
	return Hex().stringToArray(str).toByteArray();
}

static Provider *getProviderForType(const QString &type, const QString &provider)
{
	QMutexLocker lock(&global->manager_mutex);

	Provider *p = 0;
	bool scanned = false;
	if(!provider.isEmpty())
	{
		// try using specific provider
		p = global->manager.findFor(provider, type);
		if(!p)
		{
			// maybe this provider is new, so scan and try again
			global->manager.scan();
			scanned = true;
			p = global->manager.findFor(provider, type);
		}
	}
	if(!p)
	{
		// try using some other provider
		p = global->manager.findFor(QString(), type);
		if((!p || p->name() == "default") && !scanned)
		{
			// maybe there are new providers, so scan and try again
			//   before giving up or using default
			global->manager.scan();
			scanned = true;
			p = global->manager.findFor(QString(), type);
		}
	}

	return p;
}

Provider::Context *getContext(const QString &type, const QString &provider)
{
	init();

	Provider *p = getProviderForType(type, provider);
	if(!p)
		return 0;

	return p->createContext(type);
}

Provider::Context *getContext(const QString &type, Provider *p)
{
	init();

	Provider *_p = global->manager.find(p);
	if(!_p)
		return 0;

	return _p->createContext(type);
}

//----------------------------------------------------------------------------
// Initializer
//----------------------------------------------------------------------------
Initializer::Initializer(MemoryMode m, int prealloc)
{
	init(m, prealloc);
}

Initializer::~Initializer()
{
	deinit();
}

//----------------------------------------------------------------------------
// Provider
//----------------------------------------------------------------------------
Provider::~Provider()
{
}

void Provider::init()
{
}

Provider::Context::Context(Provider *parent, const QString &type)
{
	_provider = parent;
	_type = type;
}

Provider::Context::~Context()
{
}

Provider *Provider::Context::provider() const
{
	return _provider;
}

QString Provider::Context::type() const
{
	return _type;
}

bool Provider::Context::sameProvider(const Context *c) const
{
	return (c->provider() == _provider);
}

//----------------------------------------------------------------------------
// PKeyBase
//----------------------------------------------------------------------------
PKeyBase::PKeyBase(Provider *p, const QString &type)
:Provider::Context(p, type)
{
	moveToThread(0); // no thread association
}

int PKeyBase::maximumEncryptSize(EncryptionAlgorithm) const
{
	return 0;
}

QSecureArray PKeyBase::encrypt(const QSecureArray &, EncryptionAlgorithm) const
{
	return QSecureArray();
}

bool PKeyBase::decrypt(const QSecureArray &, QSecureArray *, EncryptionAlgorithm) const
{
	return false;
}

void PKeyBase::startSign(SignatureAlgorithm, SignatureFormat)
{
}

void PKeyBase::startVerify(SignatureAlgorithm, SignatureFormat)
{
}

void PKeyBase::update(const QSecureArray &)
{
}

QSecureArray PKeyBase::endSign()
{
	return QSecureArray();
}

bool PKeyBase::endVerify(const QSecureArray &)
{
	return false;
}

SymmetricKey PKeyBase::deriveKey(const PKeyBase &) const
{
	return SymmetricKey();
}

//----------------------------------------------------------------------------
// KeyStoreEntryContext
//----------------------------------------------------------------------------
KeyBundle KeyStoreEntryContext::keyBundle() const
{
	return KeyBundle();
}

Certificate KeyStoreEntryContext::certificate() const
{
	return Certificate();
}

CRL KeyStoreEntryContext::crl() const
{
	return CRL();
}

PGPKey KeyStoreEntryContext::pgpSecretKey() const
{
	return PGPKey();
}

PGPKey KeyStoreEntryContext::pgpPublicKey() const
{
	return PGPKey();
}

//----------------------------------------------------------------------------
// KeyStoreListContext
//----------------------------------------------------------------------------
bool KeyStoreListContext::isReadOnly(int) const
{
	return true;
}

bool KeyStoreListContext::writeEntry(int, const KeyBundle &)
{
	return false;
}

bool KeyStoreListContext::writeEntry(int, const Certificate &)
{
	return false;
}

bool KeyStoreListContext::writeEntry(int, const CRL &)
{
	return false;
}

PGPKey KeyStoreListContext::writeEntry(int, const PGPKey &)
{
	return PGPKey();
}

bool KeyStoreListContext::removeEntry(int, const QString &)
{
	return false;
}

void KeyStoreListContext::submitPassphrase(int, int, const QSecureArray &)
{
}

void KeyStoreListContext::rejectPassphraseRequest(int, int)
{
}

//----------------------------------------------------------------------------
// TLSContext
//----------------------------------------------------------------------------
void TLSContext::setMTU(int)
{
}

//----------------------------------------------------------------------------
// SMSContext
//----------------------------------------------------------------------------
void SMSContext::setTrustedCertificates(const CertificateCollection &)
{
}

void SMSContext::setPrivateKeys(const QList<SecureMessageKey> &)
{
}

//----------------------------------------------------------------------------
// BufferedComputation
//----------------------------------------------------------------------------
BufferedComputation::~BufferedComputation()
{
}

QSecureArray BufferedComputation::process(const QSecureArray &a)
{
	clear();
	update(a);
	return final();
}

//----------------------------------------------------------------------------
// Filter
//----------------------------------------------------------------------------
Filter::~Filter()
{
}

QSecureArray Filter::process(const QSecureArray &a)
{
	clear();
	QSecureArray buf = update(a);
	if(!ok())
		return QSecureArray();
	QSecureArray fin = final();
	if(!ok())
		return QSecureArray();
	int oldsize = buf.size();
	buf.resize(oldsize + fin.size());
	memcpy(buf.data() + oldsize, fin.data(), fin.size());
	return buf;
}

//----------------------------------------------------------------------------
// Algorithm
//----------------------------------------------------------------------------
class Algorithm::Private : public QSharedData
{
public:
	Provider::Context *c;

	Private(Provider::Context *context)
	{
		c = context;
		//printf("** [%p] Algorithm Created\n", c);
	}

	Private(const Private &from) : QSharedData(from)
	{
		c = from.c->clone();
		//printf("** [%p] Algorithm Copied (to [%p])\n", from.c, c);
	}

	~Private()
	{
		//printf("** [%p] Algorithm Destroyed\n", c);
		delete c;
	}
};

Algorithm::Algorithm()
{
	d = 0;
}

Algorithm::Algorithm(const QString &type, const QString &provider)
{
	d = 0;
	change(type, provider);
}

Algorithm::Algorithm(const Algorithm &from)
{
	d = 0;
	*this = from;
}

Algorithm::~Algorithm()
{
}

Algorithm & Algorithm::operator=(const Algorithm &from)
{
	d = from.d;
	return *this;
}

QString Algorithm::type() const
{
	if(d)
		return d->c->type();
	else
		return QString();
}

Provider *Algorithm::provider() const
{
	if(d)
		return d->c->provider();
	else
		return 0;
}

Provider::Context *Algorithm::context()
{
	if(d)
		return d->c;
	else
		return 0;
}

const Provider::Context *Algorithm::context() const
{
	if(d)
		return d->c;
	else
		return 0;
}

void Algorithm::change(Provider::Context *c)
{
	if(c)
		d = new Private(c);
	else
		d = 0;
}

void Algorithm::change(const QString &type, const QString &provider)
{
	if(!type.isEmpty())
		change(getContext(type, provider));
	else
		change(0);
}

Provider::Context *Algorithm::takeContext()
{
	if(d)
	{
		Provider::Context *c = d->c; // should cause a detach
		d->c = 0;
		d = 0;
		return c;
	}
	else
		return 0;
}

//----------------------------------------------------------------------------
// SymmetricKey
//----------------------------------------------------------------------------
SymmetricKey::SymmetricKey()
{
}

SymmetricKey::SymmetricKey(int size)
{
	set(globalRNG().nextBytes(size, Random::SessionKey));
}

SymmetricKey::SymmetricKey(const QSecureArray &a)
{
	set(a);
}

SymmetricKey::SymmetricKey(const QByteArray &a)
{
	set(QSecureArray(a));
}

/* from libgcrypt-1.2.0 */
static unsigned char desWeakKeyTable[64][8] =
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /*w*/
	{ 0x00, 0x00, 0x1e, 0x1e, 0x00, 0x00, 0x0e, 0x0e },
	{ 0x00, 0x00, 0xe0, 0xe0, 0x00, 0x00, 0xf0, 0xf0 },
	{ 0x00, 0x00, 0xfe, 0xfe, 0x00, 0x00, 0xfe, 0xfe },
	{ 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x0e }, /*sw*/
	{ 0x00, 0x1e, 0x1e, 0x00, 0x00, 0x0e, 0x0e, 0x00 },
	{ 0x00, 0x1e, 0xe0, 0xfe, 0x00, 0x0e, 0xf0, 0xfe },
	{ 0x00, 0x1e, 0xfe, 0xe0, 0x00, 0x0e, 0xfe, 0xf0 },
	{ 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xf0, 0x00, 0xf0 }, /*sw*/
	{ 0x00, 0xe0, 0x1e, 0xfe, 0x00, 0xf0, 0x0e, 0xfe },
	{ 0x00, 0xe0, 0xe0, 0x00, 0x00, 0xf0, 0xf0, 0x00 },
	{ 0x00, 0xe0, 0xfe, 0x1e, 0x00, 0xf0, 0xfe, 0x0e },
	{ 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe }, /*sw*/
	{ 0x00, 0xfe, 0x1e, 0xe0, 0x00, 0xfe, 0x0e, 0xf0 },
	{ 0x00, 0xfe, 0xe0, 0x1e, 0x00, 0xfe, 0xf0, 0x0e },
	{ 0x00, 0xfe, 0xfe, 0x00, 0x00, 0xfe, 0xfe, 0x00 },
	{ 0x1e, 0x00, 0x00, 0x1e, 0x0e, 0x00, 0x00, 0x0e },
	{ 0x1e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x0e, 0x00 }, /*sw*/
	{ 0x1e, 0x00, 0xe0, 0xfe, 0x0e, 0x00, 0xf0, 0xfe },
	{ 0x1e, 0x00, 0xfe, 0xe0, 0x0e, 0x00, 0xfe, 0xf0 },
	{ 0x1e, 0x1e, 0x00, 0x00, 0x0e, 0x0e, 0x00, 0x00 },
	{ 0x1e, 0x1e, 0x1e, 0x1e, 0x0e, 0x0e, 0x0e, 0x0e }, /*w*/
	{ 0x1e, 0x1e, 0xe0, 0xe0, 0x0e, 0x0e, 0xf0, 0xf0 },
	{ 0x1e, 0x1e, 0xfe, 0xfe, 0x0e, 0x0e, 0xfe, 0xfe },
	{ 0x1e, 0xe0, 0x00, 0xfe, 0x0e, 0xf0, 0x00, 0xfe },
	{ 0x1e, 0xe0, 0x1e, 0xe0, 0x0e, 0xf0, 0x0e, 0xf0 }, /*sw*/
	{ 0x1e, 0xe0, 0xe0, 0x1e, 0x0e, 0xf0, 0xf0, 0x0e },
	{ 0x1e, 0xe0, 0xfe, 0x00, 0x0e, 0xf0, 0xfe, 0x00 },
	{ 0x1e, 0xfe, 0x00, 0xe0, 0x0e, 0xfe, 0x00, 0xf0 },
	{ 0x1e, 0xfe, 0x1e, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe }, /*sw*/
	{ 0x1e, 0xfe, 0xe0, 0x00, 0x0e, 0xfe, 0xf0, 0x00 },
	{ 0x1e, 0xfe, 0xfe, 0x1e, 0x0e, 0xfe, 0xfe, 0x0e },
	{ 0xe0, 0x00, 0x00, 0xe0, 0xf0, 0x00, 0x00, 0xf0 },
	{ 0xe0, 0x00, 0x1e, 0xfe, 0xf0, 0x00, 0x0e, 0xfe },
	{ 0xe0, 0x00, 0xe0, 0x00, 0xf0, 0x00, 0xf0, 0x00 }, /*sw*/
	{ 0xe0, 0x00, 0xfe, 0x1e, 0xf0, 0x00, 0xfe, 0x0e },
	{ 0xe0, 0x1e, 0x00, 0xfe, 0xf0, 0x0e, 0x00, 0xfe },
	{ 0xe0, 0x1e, 0x1e, 0xe0, 0xf0, 0x0e, 0x0e, 0xf0 },
	{ 0xe0, 0x1e, 0xe0, 0x1e, 0xf0, 0x0e, 0xf0, 0x0e }, /*sw*/
	{ 0xe0, 0x1e, 0xfe, 0x00, 0xf0, 0x0e, 0xfe, 0x00 },
	{ 0xe0, 0xe0, 0x00, 0x00, 0xf0, 0xf0, 0x00, 0x00 },
	{ 0xe0, 0xe0, 0x1e, 0x1e, 0xf0, 0xf0, 0x0e, 0x0e },
	{ 0xe0, 0xe0, 0xe0, 0xe0, 0xf0, 0xf0, 0xf0, 0xf0 }, /*w*/
	{ 0xe0, 0xe0, 0xfe, 0xfe, 0xf0, 0xf0, 0xfe, 0xfe },
	{ 0xe0, 0xfe, 0x00, 0x1e, 0xf0, 0xfe, 0x00, 0x0e },
	{ 0xe0, 0xfe, 0x1e, 0x00, 0xf0, 0xfe, 0x0e, 0x00 },
	{ 0xe0, 0xfe, 0xe0, 0xfe, 0xf0, 0xfe, 0xf0, 0xfe }, /*sw*/
	{ 0xe0, 0xfe, 0xfe, 0xe0, 0xf0, 0xfe, 0xfe, 0xf0 },
	{ 0xfe, 0x00, 0x00, 0xfe, 0xfe, 0x00, 0x00, 0xfe },
	{ 0xfe, 0x00, 0x1e, 0xe0, 0xfe, 0x00, 0x0e, 0xf0 },
	{ 0xfe, 0x00, 0xe0, 0x1e, 0xfe, 0x00, 0xf0, 0x0e },
	{ 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00 }, /*sw*/
	{ 0xfe, 0x1e, 0x00, 0xe0, 0xfe, 0x0e, 0x00, 0xf0 },
	{ 0xfe, 0x1e, 0x1e, 0xfe, 0xfe, 0x0e, 0x0e, 0xfe },
	{ 0xfe, 0x1e, 0xe0, 0x00, 0xfe, 0x0e, 0xf0, 0x00 },
	{ 0xfe, 0x1e, 0xfe, 0x1e, 0xfe, 0x0e, 0xfe, 0x0e }, /*sw*/
	{ 0xfe, 0xe0, 0x00, 0x1e, 0xfe, 0xf0, 0x00, 0x0e },
	{ 0xfe, 0xe0, 0x1e, 0x00, 0xfe, 0xf0, 0x0e, 0x00 },
	{ 0xfe, 0xe0, 0xe0, 0xfe, 0xfe, 0xf0, 0xf0, 0xfe },
	{ 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf0, 0xfe, 0xf0 }, /*sw*/
	{ 0xfe, 0xfe, 0x00, 0x00, 0xfe, 0xfe, 0x00, 0x00 },
	{ 0xfe, 0xfe, 0x1e, 0x1e, 0xfe, 0xfe, 0x0e, 0x0e },
	{ 0xfe, 0xfe, 0xe0, 0xe0, 0xfe, 0xfe, 0xf0, 0xf0 },
	{ 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe }  /*w*/
};

bool SymmetricKey::isWeakDESKey()
{
	if(size() != 8)
		return false; // dubious
	QSecureArray workingCopy(8);
	// clear parity bits
	for(uint i = 0; i < 8; i++)
		workingCopy[i] = (data()[i]) & 0xfe;
	
	for(int n = 0; n < 64; n++)
	{
		if(memcmp(workingCopy.data(), desWeakKeyTable[n], 8) == 0)
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------
// InitializationVector
//----------------------------------------------------------------------------
InitializationVector::InitializationVector()
{
}

InitializationVector::InitializationVector(int size)
{
	set(globalRNG().nextBytes(size, Random::Nonce));
}

InitializationVector::InitializationVector(const QSecureArray &a)
{
	set(a);
}

InitializationVector::InitializationVector(const QByteArray &a)
{
	set(QSecureArray(a));
}

}
