package can4linux;

import java.io.Closeable;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.ClosedChannelException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.FileAttribute;
import java.nio.file.attribute.PosixFilePermission;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Objects;
import java.util.Set;

public class CAN4LinuxAdapter implements Closeable {

	static {
		final String LIB_JNI_SOCKETCAN = "jni_can4linux";
		try {
			System.loadLibrary(LIB_JNI_SOCKETCAN);
		} catch (final UnsatisfiedLinkError e) {
			try {
				loadLibFromJar(LIB_JNI_SOCKETCAN);
			} catch (final IOException _e) {
				throw new UnsatisfiedLinkError(LIB_JNI_SOCKETCAN);
			}
		}
	}

	private static void copyStream(final InputStream in, final OutputStream out) throws IOException {
		final int BYTE_BUFFER_SIZE = 0x1000;
		final byte[] buffer = new byte[BYTE_BUFFER_SIZE];
		
		for (int len; (len = in.read(buffer)) != -1;) {
			out.write(buffer, 0, len);
		}
	}

	private static void loadLibFromJar(final String libName) throws IOException {
		Objects.requireNonNull(libName);
		final String libFileName = System.mapLibraryName(libName);
		final String libJarPath = "/lib/" + libFileName;
		
		final FileAttribute<Set<PosixFilePermission>> permissions = PosixFilePermissions.asFileAttribute(PosixFilePermissions.fromString("rw-------"));
		final Path tempSo = Files.createTempFile(libFileName.substring(0, libFileName.lastIndexOf(".")), libFileName.substring(libFileName.lastIndexOf(".")), permissions);
//        File temp = File.createTempFile(prefix, suffix);
//        temp.deleteOnExit();

		
		try {
			try (final InputStream libstream = CAN4LinuxAdapter.class.getResourceAsStream(libJarPath)) {
				if (libstream == null) {
					throw new FileNotFoundException("jar:*!" + libJarPath);
				}
				try (final OutputStream fout = Files.newOutputStream(tempSo, StandardOpenOption.WRITE,
						StandardOpenOption.TRUNCATE_EXISTING)) {
					copyStream(libstream, fout);
				}
			}
			System.load(tempSo.toString());
		} finally {
			Files.delete(tempSo);
		}
	}

	private static native int canOpen(int port) throws IOException;

	private static native void canClose(int fd) throws IOException;

	private static native void canSend(int fd, boolean ext, int id, boolean rtr, byte[] data) throws IOException;

	private static native CANMessage canRead(int fd, int timeout, boolean filterSelf) throws IOException;

	private int _fd= -1;
	private final int port;
	private boolean filterSelf= true;

	public CAN4LinuxAdapter(int port, boolean filterSelf) throws IOException, IndexOutOfBoundsException {
		this(port);
		this.filterSelf= filterSelf;
	}
	
	public CAN4LinuxAdapter(int port) throws IOException, IndexOutOfBoundsException {
		if(port<0 || port>9) throw new IndexOutOfBoundsException("port number needs to be between 0 and 9");
		
		this.port = port;
		_fd = canOpen(port);
	}

	public void send(CANMessage msg) throws IOException {
		if(_fd<0) throw new IOException("CAN device is already closed");
		
		if(!msg.err)
			canSend(_fd, msg.ext, msg.id, msg.rtr, msg.data);
	}

	public CANMessage read(int timeout) throws IOException {
		if(_fd<0) throw new IOException("CAN device is already closed");

		return canRead(_fd, timeout, filterSelf);
	}

	@Override
	public void close() throws IOException {
		if(_fd<0) throw new IOException("CAN device is already closed");

		canClose(_fd);
		_fd= -1;
	}

	public boolean isFilterSelf() {
		return filterSelf;
	}

	public void setFilterSelf(boolean filterSelf) {
		this.filterSelf = filterSelf;
	}
}
