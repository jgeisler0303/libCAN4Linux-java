package can4linux;

public class CANMessage {
	static public final int CAN_SFF_MASK= 0x000007FF;
	static public final int CAN_EFF_MASK= 0x1FFFFFFF;
	
	public int id;
	public boolean rtr;
	public boolean ext;
	public boolean err;
	public int flags;
	
	public CANMessage(int id, boolean rtr, byte[] data) {
		super();
		this.id = id & CAN_SFF_MASK;
		this.rtr = rtr;
		this.ext = false;
		this.err = false;
		this.data = data;
		this.flags= 0;
	}

	public CANMessage(int id, boolean rtr, boolean ext, byte[] data) {
		super();
		if(ext) id&= CAN_SFF_MASK;
		else  id&= CAN_EFF_MASK;

		this.id = id;
		this.rtr = rtr;
		this.ext = ext;
		this.err = false;
		this.data = data;
		this.flags= 0;
	}

	public CANMessage(int id, boolean rtr, boolean ext, boolean err, byte[] data, int flags) {
		super();
		if(ext) id&= CAN_SFF_MASK;
		else  id&= CAN_EFF_MASK;

		this.id = id;
		this.rtr = rtr;
		this.ext = ext;
		this.err = err;
		this.data = data;
		this.flags= flags;
	}

	public byte[] data;
	
	public String toString() {
		StringBuffer sb = new StringBuffer();

		if(ext)
        	sb.append(String.format("E%06X", id));
        else
        	sb.append(String.format("%06X", id));

        if(rtr)
        	sb.append("R");
        	
    	sb.append("#");
        sb.append(canDataToString(data));
		return sb.toString();		
	}

	public static String canDataToString(byte[] data) {
		if (data == null) {
			return "[]";
		}
		StringBuffer sb = new StringBuffer();
		sb.append("[");
		for (int i = 0; i < data.length; i++) {
			sb.append(String.format("%02X ", data[i]));
			if(i<data.length-1)
				sb.append(" ");				
		}
		sb.append("]");
		
		return sb.toString();
	}
	
}
