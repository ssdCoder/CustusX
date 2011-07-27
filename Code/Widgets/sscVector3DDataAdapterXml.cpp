/*
 * sscVector3DDataAdapterXml.cpp
 *
 *  Created on: Jul 25, 2011
 *      Author: christiana
 */

#include <sscVector3DDataAdapterXml.h>


#include <iostream>
#include <QDomElement>
#include <QStringList>
#include "sscTypeConversions.h"

namespace ssc
{


/** Make sure one given option exists witin root.
 * If not present, fill inn the input defaults.
 */
Vector3DDataAdapterXmlPtr Vector3DDataAdapterXml::initialize(const QString& uid,
    QString name,
    QString help,
    Vector3D value,
    DoubleRange range,
    int decimals,
    QDomNode root)
{
	Vector3DDataAdapterXmlPtr retval(new Vector3DDataAdapterXml());
	retval->mUid = uid;
	retval->mName = name.isEmpty() ? uid : name;
	retval->mHelp = help;
	retval->mRange = range;
	retval->mStore = XmlOptionItem(uid, root.toElement());
	retval->mValue = Vector3D::fromString(retval->mStore.readValue(qstring_cast(Vector3D(0,0,0))));
	retval->mDecimals = decimals;
	return retval;
}


Vector3DDataAdapterXml::Vector3DDataAdapterXml()
{
  mFactor = 1.0;
}

void Vector3DDataAdapterXml::setInternal2Display(double factor)
{
  mFactor = factor;
}

QString Vector3DDataAdapterXml::getUid() const
{
	return mUid;
}

QString Vector3DDataAdapterXml::getValueName() const
{
	return mName;
}

QString Vector3DDataAdapterXml::getHelp() const
{
	return mHelp;
}

Vector3D Vector3DDataAdapterXml::getValue() const
{
  return mValue;
}

bool Vector3DDataAdapterXml::setValue(const Vector3D& val)
{
	if (ssc::similar(val,mValue))
		return false;

//	std::cout << "set val " << "  " << val << "  , org=" << mValue << std::endl;

	mValue = val;
	mStore.writeValue(qstring_cast(val));
	emit valueWasSet();
	emit changed();
	return true;
}

DoubleRange Vector3DDataAdapterXml::getValueRange() const
{
	return mRange;
}

void Vector3DDataAdapterXml::setValueRange(DoubleRange range)
{
  mRange = range;
  emit changed();
}

int Vector3DDataAdapterXml::getValueDecimals() const
{
	return mDecimals;
}



} // namespace ssc
