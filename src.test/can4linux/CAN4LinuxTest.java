package can4linux;

import java.io.IOException;

public class CAN4LinuxTest {

	public static void main(String[] args) throws IndexOutOfBoundsException, IOException {
		System.out.println(System.getProperty("os.arch"));
		System.out.println(System.getProperty("os.name"));
		
		System.out.println("opening can bus 0");
		CAN4LinuxAdapter can= new CAN4LinuxAdapter(0);
		
		System.out.println("sending message");
		can.send(new CANMessage(0x023, false, new byte[] {1, 2, 3, 4, 5, 6, 7, 8}));

		for(int i= 0; i<100; i++) {
			if(i%10==0) {
				System.out.print("waiting for message "); 
				System.out.println(i/10);
			}
			CANMessage msg= can.read(100000);
			if(msg!=null)
				System.out.println(msg.toString());
		}
		
		System.out.println("closing can bus");
		can.close();
	}

}
