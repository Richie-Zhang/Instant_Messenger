

void P(struct Semaphore *s){
	disable_interrupt();
	s->token_available --;
	if (s->token_available < 0){
		queue_add(s);
    		block_itself();
	}
	enable_interrupt();
}
void V(struct Semaphore *s){
	disable_interrupt();
	s->token_available ++;
	if (s->token_available <= 0) {
    		struct PCB *pcb = dequeue_from(s);
    		make_runnable(pcb);
	} 	
	enable_interrupt();
}
