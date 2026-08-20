import java.util.*;

class Customer {
    int token;
    String name;
    boolean vip;

    Customer(int token, String name, boolean vip) {
        this.token = token;
        this.name = name;
        this.vip = vip;
    }

    @Override
    public String toString() {
        return "Token: " + token + " | Name: " + name +
                (vip ? " (VIP)" : " (Normal)");
    }
}

class Bank {

    private Queue<Customer> normalQueue = new LinkedList<>();

    private PriorityQueue<Customer> vipQueue =
            new PriorityQueue<>(Comparator.comparingInt(c -> c.token));

    private int token = 1001;

    public void addCustomer(String name, boolean vip) {

        Customer customer = new Customer(token++, name, vip);

        if (vip) {
            vipQueue.offer(customer);
            System.out.println("VIP Customer Added");
        } else {
            normalQueue.offer(customer);
            System.out.println("Normal Customer Added");
        }

        System.out.println(customer);
    }

    public void serveCustomer() {

        if (!vipQueue.isEmpty()) {
            System.out.println("\nServing VIP Customer");
            System.out.println(vipQueue.poll());
        }
        else if (!normalQueue.isEmpty()) {
            System.out.println("\nServing Normal Customer");
            System.out.println(normalQueue.poll());
        }
        else {
            System.out.println("\nNo customers waiting.");
        }
    }

    public void displayQueues() {

        System.out.println("\n===== VIP Queue =====");

        if (vipQueue.isEmpty())
            System.out.println("Empty");
        else
            vipQueue.forEach(System.out::println);

        System.out.println("\n===== Normal Queue =====");

        if (normalQueue.isEmpty())
            System.out.println("Empty");
        else
            normalQueue.forEach(System.out::println);
    }
}

public class BankingQueueSimulator {

    public static void main(String[] args) {

        Scanner sc = new Scanner(System.in);
        Bank bank = new Bank();

        while (true) {

            System.out.println("\n===== BANK MENU =====");
            System.out.println("1. Add Normal Customer");
            System.out.println("2. Add VIP Customer");
            System.out.println("3. Serve Customer");
            System.out.println("4. Display Queue");
            System.out.println("5. Exit");

            System.out.print("Enter choice: ");
            int choice = sc.nextInt();
            sc.nextLine();

            switch (choice) {

                case 1:
                    System.out.print("Enter Customer Name: ");
                    bank.addCustomer(sc.nextLine(), false);
                    break;

                case 2:
                    System.out.print("Enter VIP Customer Name: ");
                    bank.addCustomer(sc.nextLine(), true);
                    break;

                case 3:
                    bank.serveCustomer();
                    break;

                case 4:
                    bank.displayQueues();
                    break;

                case 5:
                    System.out.println("Thank You!");
                    sc.close();
                    return;

                default:
                    System.out.println("Invalid Choice!");
            }
        }
    }
}
