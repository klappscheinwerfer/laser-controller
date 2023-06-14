#ifndef PARSE_H
#define PARSE_H

#include <QString>
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QtXml>

QVector<QString> ncToCmd(QString filename) {
	QVector<QString> result;
	QFile file(filename);

	if(!file.open(QIODevice::ReadOnly)) {
		QMessageBox::information(0, "error", file.errorString());
	}

	QTextStream in(&file);

	while(!in.atEnd()) {
		QString line = in.readLine();
		line += '\n';
		result.append(line);
	}

	file.close();
	return result;
}

QVector<QString> svgToCmd(QString filename) {
	QVector<QString> result;
	QDomDocument svgdoc;
	QFile file(filename);

	if(!file.open(QIODevice::ReadOnly)) {
		QMessageBox::information(0, "error", file.errorString());
	}
	
	svgdoc.setContent(&file);
	file.close();

	QDomNodeList nodes = svgdoc.elementsByTagName("path");

	for(int i = 0; i < nodes.count(); i++) {
		QDomNode path = nodes.at(i);

		if (!path.isElement()) continue;

		QString d = path.toElement().attributes().namedItem("d").nodeValue();
		qDebug() << "Path" << d;

		float rx = 0; // Relative X
		float ry = 0; // Relative Y

		// Split path into commands
		QVector<QString> commands;
		for (int i = 0; i < d.length(); i++) {
			if (d[i].isLetter()) commands.append(QString(d[i]));
			else if (d[i].isSpace());
			else commands.last() += QString(d[i]);
		}

		for (QString cmd : commands) {
			char cc = cmd[0].toLatin1();
			cmd.remove(0, 1);
			QStringList args = cmd.split(',');

			float dx, dy; // Delta
			switch (cc) {
				case 'm': // Relative move
					dx = args[0].toFloat();
					dy = args[1].toFloat();
					result.append("G01 X" + args[0] + " Y" + args[1] + "\n");
					rx += dx;
					ry += dy;
					break;
				case 'l': // Relative line
					dx = args[0].toFloat();
					dy = args[1].toFloat();
					result.append("M03\n");
					result.append("G01 X" + args[0] + " Y" + args[1] + "\n");
					result.append("M05\n");
					rx += dx;
					ry += dy;
					break;
				case 'h': // Horizontal line
					dy = args[0].toFloat();
					result.append("M03\n");
					result.append("G01 X0 Y" + args[0] + "\n");
					result.append("M05\n");
					ry += dy;
					break;
				case 'v': // Vertical line
					dx = args[0].toFloat();
					result.append("M03\n");
					result.append("G01 X" + args[0] + " Y0\n");
					result.append("M05\n");
					rx += dx;
					break;
				case 'Z':
				case 'z': // Close path
					qDebug() << 'z';
					result.append("G01 X" + QString::number(-rx) + " Y" + QString::number(-ry) + "\n");
					break;
				default: // Unrecognized character
					break;
			}
		}
	}

	return result;
}

// Select attribute by character code and multiply it by a given value
QString multiplyAttribute(QString str, char code, float multiplier) {
	int f = 0, l; // markers
	while (str[f] != code)
		if (f++ == str.size() - 1) return str; // Character not found
	for (l = f++; l < str.size() && str[l] != ' ' && str[l] != '\n'; l++);
	float nf = str.mid(f, l - f).toFloat() * multiplier; // New value
	str.remove(f, l - f);
	str.insert(f, QString::number(nf));
	return str;
}

// Generate a move command with attributes
QString moveCmd(float x, float y, float f) {
	return "G01 X" + QString::number(x) + " Y" + QString::number(y) + " F" + QString::number(f) + "\n";
}

#endif // PARSE_H