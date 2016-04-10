
// Copyright (C) 1987 - 2008
// H. N. Oredson

// RoundTable server.


#include "stdafx.h"
//#include "global.h"

#define deftopic "General"
#define ln_ibuf 128

// Protocol version.

#define FORMAT       1
#define FORMAT_O     0   // Offset in frame to format byte.
#define TYPE_O       1   // Offset in frame to kind byte.
#define DATA_O       2   // Offset in frame to data.

// Protocol Frame Types.

#define id_join   'J'    // User joins RT.
#define id_leave  'L'    // User leaves RT.
#define id_link   'N'    // Node joins RT.
#define id_unlink 'Q'    // Node leaves RT.
#define id_data   'D'    // Data for all users.
#define id_send   'S'    // Data for one user.
#define id_topic  'T'    // User changes topic.
#define id_user   'I'    // User login information.

// RT protocol version 1.
// First two bytes are FORMAT and Frame Type.
// These are followed by text fields delimited by blanks.
// Note that "node", "to", "from", "user" are callsigns.

// ^AD<node> <user> <text>        - Data for all users.
// ^AI<node> <user> <name> <qth>  - User information.
// ^AJ<node> <user> <name> <qth>  - User joins.
// ^AL<node> <user> <name> <qth>  - User leaves.
// ^AN<node> <node> <alias>       - Node joins.
// ^AQ<node> <node>               - Node leaves.
// ^AS<node> <from> <to>   <text> - Data for one user.
// ^AT<node> <user> <topic>       - User changes topic.

// Connect protocol:

// 1. Connect to node.
// 2. Send *RTL
// 3. Receive OK. Will get disconnect if link is not allowed.
// 4. Go to it.

// Disconnect protocol:

// 1. If there are users on this node, send an id_leave for each user,
//    to each node you are disconnecting from.
// 2. Disconnect.

// Other RT systems to link with. Flags can be p_linked, p_linkini.

typedef struct link_t
{
	struct link_t *next;
	char *alias;
	char *call;
	int  flags; // See circuit flags.
} LINK;

static LINK *link_hd = NULL;

// Topics.

typedef struct topic_t
{
	struct topic_t *next;
	char  *name;
	int  refcnt;
} TOPIC;

static TOPIC *topic_hd = NULL;

// Nodes.

typedef struct node_t
{
	struct node_t *next;
	char *alias;
	char *call;
	int refcnt;
} NODE;

static NODE *node_hd = NULL;

// Topics in use at each circuit.

typedef struct ct_t
{
	struct ct_t *next;
	TOPIC *topic;
	int  refcnt;
} CT;

// Nodes reached on each circuit.

typedef struct cn_t
{
	struct cn_t *next;
	NODE *node;
	int refcnt;
} CN;

// Circuits.
// A circuit may be used by one local user, or one link.
// If it is used by a link, there may be many users on that link.

// Bits for circuit flags and link flags.

#define p_nil     0x00    // Circuit is being shut down.
#define p_user    0x01    // User connected.
#define p_linked  0x02    // Active link with another RT.
#define p_linkini 0x04    // Outgoing link setup with another RT.

typedef struct circuit_t
{
	struct circuit_t *next;
	PROC *proc;
	byte flags;             // p_linked or p_user.
	int s;                 // Socket.
	char buf[ln_ibuf];      // Line of incoming text.
	union
	{
		struct user_t *user;  // Associated user if local.
		LINK *link;           // Associated link if link.
	} u;
	int refcnt;            // If link, # of users on that link.
	CN   *hnode;            // Nodes heard from this link.
	CT   *topic;            // Out this circuit if from these topics.
} CIRCUIT;

static CIRCUIT *circuit_hd = NULL;

// Users. Could be connected at any node.

#define u_echo 0x0002     // User wants his text echoed to him.

typedef struct user_t
{
	struct  user_t *next;
	char    *call;
	char    *name;
	char    *qth;
	NODE    *node;          // Node user logged into.
	CIRCUIT *circuit;       // Circuit user is on, local or link.
	TOPIC   *topic;         // Topic user is in.
	int     flags;
} USER;

static USER *user_hd = NULL;

static PROC *Rt_Control;
static int  rtrun = FALSE;

#define rtjoin  "*** Joined the RoundTable"
#define rtleave "*** Left the RoundTable"

// Flush any pending RT output if the send queue is not full.
// If the send queue is full, the nflush would block.

static void rt_control ()
{
	CIRCUIT *circuit;
	SOCKET  *up;

	puser("System");

	for (;;)
	{
		pause(250);

// RT has shut down?

		if (!circuit_hd) { rtrun = FALSE; return; }

		if (!rtrun)
		{
			for (circuit = circuit_hd; circuit; circuit = circuit->next)
				alert(circuit->proc);
			return;
		}

		for (circuit = circuit_hd; circuit; circuit = circuit->next)
		{
			up = find_s(circuit->s);
			if (up && up->cb.p && !txqfull(up)) nflush(circuit->s);
		}
	}
}

static void upduser(USER *user)
{
	FILE *in, *out;
	char *c, *buf;

	if (!nfopen2(RtUsr, &in, RtUsr_t, &out)) return;
	buf = mallocw(LINE128);

  while(fgets(buf, LINE128, in))
	{
 	  c = strchr(buf, ' ');
 	  if (c) *c = '\0';
		if (!matchi(buf, user->call))	{ if (c) *c = ' '; fputs(buf, out); }
	}

	free(buf);
	fprintf(out, "%s %s %s\n", user->call, user->name, user->qth);
	nfclose2(RtUsr, &in, RtUsr_t, &out);
}

static void rduser(USER *user)
{
	FILE *in;
	char *buf, *name, *qth;

	user->name = strdup("?_name");
	user->qth  = strdup("?_qth");

	in = nfopeni(RtUsr, "r");

	if (in)
	{
		buf = mallocw(LINE128);

	  while(fgets(buf, LINE128, in))
	  {
		  rip(buf);
	    name = strlop(buf, ' ');
			if (!matchi(buf, user->call)) continue;
			if (!name) break;
			qth = strlop(name, ' ');
			strnew(&user->name, name);
			if (!qth) break;
			strnew(&user->qth,  qth);
			break;
		}

		free(buf);
		nfclose(&in);
	}
}

static void circuit_free(CIRCUIT *circuit)
{
	CIRCUIT *c, *cp;
	CN      *ncn;
	NODE    *nn;
	TOPIC   *tn;

	cp = NULL;

	for (c = circuit_hd; c; cp = c, c = c->next) if (c == circuit)
	{
		if (cp) cp->next = c->next; else circuit_hd = c->next;

		while (c->hnode)
		{
			ncn = c->hnode->next;
			free(c->hnode);
			c->hnode = ncn;
		}

		free(c);
		break;
	}

	if (circuit_hd) return;

// RT has gone inactive. Clean up.

	while (node_hd)
	{
		nn = node_hd->next;
		free(node_hd->alias);
		free(node_hd->call);
		free(node_hd);
		node_hd = nn;
	}

	while (topic_hd)
	{
		tn = topic_hd->next;
		free(topic_hd->name);
		free(topic_hd);
		topic_hd = tn;
	}
}

// Create a circuit.

static CIRCUIT *circuit_new(int flags, int s)
{
	CIRCUIT *circuit;

	if (MemLow) return NULL;

// If RT is starting up, fire up the control process.

	if (!circuit_hd)
		Rt_Control = pcreate("RT_Control", RT_STACK, rt_control, PR_NONE, NULL);

	if (!Rt_Control) return NULL;

	circuit = zallocw(sizeof(CIRCUIT));
	sl_ins_hd(circuit, circuit_hd);
	circuit->s     = s;
	circuit->flags = flags;
	circuit->proc  = CurProc;
	return circuit;
}

// Find a node in the node list.

static NODE *node_find(char *call)
{
	NODE *node;

	for (node = node_hd; node; node = node->next)
		if (node->refcnt && matchi(node->call, call)) break;

	return node;
}

// Add a reference to a node.

static NODE *node_inc(char *call, char *alias)
{
	NODE *node;

	node = node_find(call);

	if (!node)
	{
		node = zallocw(sizeof(NODE));
		sl_ins_hd(node, node_hd);
		node->call  = strdup(call);
		node->alias = strdup(alias);
	}

	node->refcnt++;
	return node;
}

// Remove a reference to a node.

static void node_dec(NODE *node)
{
	NODE *t, *tp;

	if (--node->refcnt) return; // Other references.

// Remove the node from the node list.

	tp = NULL;

	for (t = node_hd; t; tp = t, t = t->next) if (t == node)
	{
		if (tp) tp->next = t->next; else node_hd = t->next;
		free(t->alias);
		free(t->call);
		free(t);
		break;
	}
}

// User joins a topic.

static TOPIC *topic_join(CIRCUIT *circuit, char *s)
{
	CT    *ct;
	TOPIC *topic;

// Look for an existing topic.

	for (topic = topic_hd; topic; topic = topic->next)
		if (matchi(topic->name, s)) break;

// Create a new topic, if needed.

	if (!topic)
	{
		topic = zallocw(sizeof(TOPIC));
		sl_ins_hd(topic, topic_hd);
		topic->name = strdup(s);
	}

	topic->refcnt++;  // One more user in this topic.

// Add the circuit / topic association.

	for (ct = circuit->topic; ct; ct = ct->next) if (ct->topic == topic)
	{
	  ct->refcnt++;
	  return topic;
	}

	ct = zallocw(sizeof(CT));
	sl_ins_hd(ct, circuit->topic);
	ct->topic  = topic;
	ct->refcnt = 1;
	return topic;
}

// User leaves a topic.

static void topic_leave(CIRCUIT *circuit, TOPIC *topic)
{
	CT    *ct, *ctp;
	TOPIC *t,  *tp;

	topic->refcnt--;

	ctp = NULL;

	for (ct = circuit->topic; ct; ctp = ct, ct = ct->next) if (ct->topic == topic)
	{
	  if (!--ct->refcnt)
	  {
	  	if (ctp) ctp->next = ct->next; else circuit->topic = ct->next;
			free(ct);
		}

		tp = NULL;

		for (t = topic_hd; t; t = t->next) if (!t->refcnt && (t == topic))
		{
			if (tp) tp->next = t->next; else topic_hd = t->next;
			free(t->name);
			free(t);
			break;
		}
	}
}

// Find a circuit/topic association.

static int ct_find(CIRCUIT *circuit, TOPIC *topic)
{
	CT *ct;

	for (ct = circuit->topic; ct; ct = ct->next)
		if (ct->topic == topic) return ct->refcnt;

	return 0;
}

// Nodes reached from each circuit. Used only if the circuit is a link.

// Remove a circuit/node association.

static void cn_dec(CIRCUIT *circuit, NODE *node)
{
	CN *c, *cp;

	node_dec(node);

	cp = NULL;

	for (c = circuit->hnode; c; cp = c, c = c->next) if (c->node == node)
	{
	  if (--c->refcnt) return;
	  if (cp) cp->next = c->next; else circuit->hnode = c->next;
		free(c);
	}
}

// Add a circuit/node association.

static NODE *cn_inc(CIRCUIT *circuit, char *call, char *alias)
{
	NODE *node;
	CN *cn;

	node = node_inc(call, alias);

	for (cn = circuit->hnode; cn; cn = cn->next) if (cn->node == node)
	{
	  cn->refcnt++;
	  return node;
	}

	cn = zallocw(sizeof(CN));
	sl_ins_hd(cn, circuit->hnode);
	cn->node   = node;
	cn->refcnt = 1;
	return node;
}

// Find a circuit/node association.

static int cn_find(CIRCUIT *circuit, NODE *node)
{
	CN *cn;

	for (cn = circuit->hnode; cn; cn = cn->next) if (cn->node == node)
		return cn->refcnt;

	return 0;
}

// From a local user to a specific user at another node.

static void text_xmit(USER *user, USER *to, char *text)
{
	nprintf(to->circuit->s, "%c%c%s %s %s %s\n",
		FORMAT, id_send, Node->calls, user->call, to->call, text);
}


// From a local or remote user to local user(s).

#define o_all    1  // To all users.
#define o_one    2  // To a specific user.
#define o_topic  3  // To all users in a specific topic.

static void text_tellu(USER *user, char *text, char *to, int who)
{
	CIRCUIT *circuit;
	char    *buf;

	buf = mallocw(strlen(text) + 11);
	sprintf(buf, "%-6.6s %c %s\n", user->call, (who == o_one) ? '>' : ':', text);

// Send it to all connected users in the same topic.
// Echo to originator if requested.

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (!(circuit->flags & p_user)) continue;  // Circuit is a link.
		if ((circuit->u.user == user) && !(user->flags & u_echo)) continue;

		switch(who)
		{
			case o_topic :
				if (circuit->u.user->topic == user->topic) nputs(circuit->s, buf);
				break;
			case o_all :
				nputs(circuit->s, buf);
				break;
			case o_one :
				if (matchi(circuit->u.user->call, to)) nputs(circuit->s, buf);
				break;
		}
	}

	free(buf);
}

// Tell one link circuit about a local user change of topic.

static void topic_xmit(USER *user, CIRCUIT *circuit)
{
	nprintf(circuit->s, "%c%c%s %s %s\n",
		FORMAT, id_topic, Node->calls, user->call, user->topic->name);
}

// Tell another node about one known node on a link add or drop
// if that node is from some other link.

static void node_xmit(NODE *node, char kind, CIRCUIT *circuit)
{
	if (!cn_find(circuit, node)) nprintf(circuit->s, "%c%c%s %s %s\n",
		FORMAT, kind, Node->calls, node->call, node->alias);
}

// Tell all other nodes about one node known by this node.

static void node_tell(NODE *node, char kind)
{
	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
		if (circuit->flags & p_linked) node_xmit(node, kind, circuit);
}

// Tell another node about a user login/logout at this node.

static void user_xmit(USER *user, char kind, CIRCUIT *circuit)
{
	NODE *node;

	node = user->node;
	if (!cn_find(circuit, node)) nprintf(circuit->s, "%c%c%s %s %s %s\n",
		FORMAT, kind, node->call, user->call, user->name, user->qth);
}

// Tell all other nodes about a user login/logout at this node.

static void user_tell(USER *user, char kind)
{
	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
		if (circuit->flags & p_linked)
	  user_xmit(user, kind, circuit);
}

// Find the user record for call.

static USER *user_find(char *call)
{
	USER *user;

	for (user = user_hd; user; user = user->next)
		if (matchi(user->call, call)) break;

	return user;
}

static void user_leave(USER *user)
{
	USER *t, *tp;

	topic_leave(user->circuit, user->topic);

	tp = NULL;

	for (t = user_hd; t; tp = t, t = t->next) if (t == user)
	{
		if (tp) tp->next = t->next; else user_hd = t->next;
		free(t->name);
		free(t->call);
		free(t->qth);
		free(t);
		break;
	}
}

// User changed to a different topic.

static void topic_chg(USER *user, char *s)
{
	char *buf;

  buf = mallocw(max(strlen(user->topic->name), strlen(s)) + 18);
	sprintf(buf, "*** Left Topic: %s", user->topic->name);
	text_tellu(user, buf, NULL, o_topic); // Tell everyone in the old topic.
	topic_leave(user->circuit, user->topic);
	user->topic = topic_join(user->circuit, s);
	sprintf(buf, "*** Joined Topic: %s", user->topic->name);
	text_tellu(user, buf, NULL, o_topic); // Tell everyone in the new topic.
	free(buf);
}

// Create a user record for this user.

static USER *user_join(CIRCUIT *circuit, char *ucall, char *ncall, char *nalias)
{
	NODE *node;
	USER *user;

	node = cn_inc(circuit, ncall, nalias);

// Is this user already logged in at this node?

	for (user = user_hd; user; user = user->next)
		if (matchi(user->call, ucall) && (user->node == node)) return user;

// User is not logged in, create a user record for them.

	user = zallocw(sizeof(USER));
	sl_ins_hd(user, user_hd);
	user->circuit = circuit;
	user->topic   = topic_join(circuit, deftopic);
	user->flags   = u_echo;
	user->call    = strdup(ucall);
	strupr(user->call);
	user->node    = node;
	rduser(user);
	if (circuit->flags & p_user) circuit->u.user = user;
	return user;
}

// Link went away. We dropped it, or the other node dropped it.
// Drop nodes and users connected from this link.
// Tell other (still connected) links what was dropped.

static void link_drop(CIRCUIT *circuit)
{
	USER *user;
	CN   *cn;

// So we don't try and send anything on this circuit.

	circuit->u.link->flags = p_nil;
	circuit->flags = p_nil;

// Users connected on the dropped link are no longer connected.

	for (user = user_hd; user; user = user->next) if (user->circuit == circuit)
	{
		circuit->refcnt--;
		node_dec(user->node);
		text_tellu(user, rtleave, NULL, o_all);
		user_tell(user, id_leave);
		user_leave(user);
	}

// Any node known from the dropped link is no longer known.

	for (cn = circuit->hnode; cn; cn = cn->next)
	{
		node_tell(cn->node, id_unlink);
		node_dec(cn->node);
	}

// The circuit is no longer used.

	circuit_free(circuit);
}

// Handle an incoming control frame from a linked RT system.

static void echo(CIRCUIT *fc, NODE *node)
{
	CIRCUIT *tc;

	for (tc = circuit_hd; tc; tc = tc->next)
	if ((tc isnt fc) && (tc->flags & p_linked) && !cn_find(tc, node))
		nprintf(tc->s, "%s\n", fc->buf);
}

static void chkctl(CIRCUIT *ckt_from)
{
	NODE    *node, *ln;
	CIRCUIT *ckt_to;
	USER    *user, *su;
	char    *ncall, *ucall, *f1, *f2, *buf;

	if (ckt_from->buf[FORMAT_O] != FORMAT) return; // Not a control message.
	buf = strdup(ckt_from->buf + DATA_O);

// FORMAT and TYPE bytes are followed by node and user callsigns.

	ncall = buf;
	ucall = strlop(buf, ' ');
	if (!ucall) { free(buf); return; } // Not a control message.

// There may be at least one field after the node and user callsigns.
// Node leave (id_unlink) has no F1.

	f1 = strlop(ucall, ' ');

// If the frame came from an unknown node ignore it.
// If the frame came from us ignore it (loop breaking).

	node = node_find(ncall);
	if (!node || matchi(ncall, Node->calls)) { free(buf); return; }

	switch(ckt_from->buf[TYPE_O])
	{
// Data from user ucall at node ncall.

		case id_data :
			user = user_find(ucall);
			if (!user) break;
			text_tellu(user, f1, NULL, o_topic);

			for (ckt_to = circuit_hd; ckt_to; ckt_to = ckt_to->next)
				if ((ckt_to->flags & p_linked) &&
					   ckt_to->refcnt &&
					   !cn_find(ckt_to, node) &&
					   ct_find(ckt_to, user->topic)) nprintf(ckt_to->s, "%s\n", ckt_from->buf);
			break;

// User ucall at node ncall changed their Name/QTH info.

		case id_user :
			echo(ckt_from, node);  // Relay to other nodes.
			user = user_find(ucall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			break;

// User ucall logged into node ncall.

		case id_join :
			echo(ckt_from, node);  // Relay to other nodes.
			f2 = strlop(f1, ' ');
			if (!f2) break;
			user = user_join(ckt_from, ucall, ncall, NULL);
			if (!user) break;
			ckt_from->refcnt++;
			text_tellu(user, rtjoin, NULL, o_all);
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			break;

// User ucall logged out of node ncall.

		case id_leave :
			echo(ckt_from, node);  // Relay to other nodes.
			user = user_find(ucall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			text_tellu(user, rtleave, NULL, o_all);
			ckt_from->refcnt--;
			cn_dec(ckt_from, node);
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			user_leave(user);
			break;

// Node ncall lost its link to node ucall, alias f1.

		case id_unlink :
			echo(ckt_from, node);  // Relay to other nodes.
			ln = node_find(ucall);
			if (ln)	cn_dec(ckt_from, ln);
			break;

// Node ncall acquired a link to node ucall, alias f1.
// If we are not linked, is no problem, don't link.
// If we are linked, is a loop, do what?

		case id_link :
			echo(ckt_from, node);  // Relay to other nodes.
			ln = node_find(ucall);
			if (!ln && !matchi(ncall, Node->calls)) cn_inc(ckt_from, ucall, f1);
			break;

// User ucall at node ncall sent f2 to user f1.

		case id_send :
			user = user_find(ucall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			su = user_find(f1);
			if (!su) break;

			if (su->circuit->flags & p_user)
				text_tellu(user, f2, f1, o_one);
			else
				echo(ckt_from, node);  // Relay to other nodes.
			break;

// User ucall at node ncall changed topic.

		case id_topic :
			echo(ckt_from, node);  //  Relay to other nodes.
			user = user_find(ucall);
			if (user) topic_chg(user, f1);
			break;

		default :  break;
	}

	free(buf);
}

// Tell another node about nodes known by this node.
// Do not tell it about this node, the other node knows who it
// linked to (or who linked to it).
// Tell another node about users known by this node.
// Done at incoming or outgoing link establishment.

static void state_tell(CIRCUIT *circuit)
{
	NODE *node;
	USER *user;

	node = cn_inc(circuit, circuit->u.link->call, circuit->u.link->alias);
	node_tell(node, id_link); // Tell other nodes about this new link

// Tell the node that just linked here about nodes known on other links.

	for (node = node_hd; node; node = node->next)
	  if (!matchi(node->call, Node->calls)) node_xmit(node, id_link, circuit);

// Tell the node that just linked here about known users, and their topics.

	for (user = user_hd; user; user = user->next)
	{
		user_xmit(user, id_join, circuit);
		topic_xmit(user, circuit);
	}
}

static int getinp(CIRCUIT *circuit)
{
	if (!rxlinet(circuit->buf, ln_ibuf)) return FALSE;
	rip(circuit->buf);
	return true;
}

// Make an outgoing link.

static void link_out ()
{
	LINK    *link;
	NR_NODE *np;
	CIRCUIT *circuit;
	SADDR    addr;
	word     s;

	link = (LINK *)p;
	if (MemLow) { link->flags = p_nil; return; }
	puser(link->alias);

// See if the requested destination is a known alias or call,
// use it if it is. Otherwise give up.

	np = nr_findca(link->call);
	if (!np) { link->flags = p_nil; return; }

	s = socket(TYPE_NETROM);
	mysock(s);
	flushoff(s);  // The RT control process will do the flush.

// Set up the local and remote addresses.

	addr.type = TYPE_NETROM;
	memcpy(addr.a.nr.user, Node->call, ln_call+1);
	memcpy(addr.a.nr.node, Node->call, ln_call+1);
	bind(s, &addr);
	memcpy(addr.a.nr.user, np->call, ln_call+1);
	memcpy(addr.a.nr.node, np->call, ln_call+1);

	if (!connect(s, &addr)) { link->flags = p_nil; return; }

// Create a circuit for this link.

	circuit = circuit_new(p_linkini, s);
	if (!circuit) { link->flags = p_nil; return; }

	circuit->u.link = link;
	tputs("*RTL\n");  // Log in to the remote RT system.

// Run in circles, scream and shout.

	for (;;) if (getinp(circuit))
	{
		if (circuit->flags & p_linked)
			chkctl(circuit);
		else
		{
			if (!matchi(circuit->buf, "OK"))
			{
				link_drop(circuit);
				link->flags = p_nil;
				return;
			}

			link->flags    = p_linked;
 	  	circuit->flags = p_linked;
			state_tell(circuit);
		}
	}
	else
	{
		link_drop(circuit);
		link->flags = p_nil;
		return;
	}
}

// Link to other RT systems. Create a process for each outgoing link.
// While setting up a link, watch for incoming link requests
// from that node, abort the link setup if one is received.
// Also watch for incoming link advice (other node already linked
// to the node we are attempting to link with) and abort.

static void makelinks(void)
{
	LINK *link;

	rtrun = true;

// Make the links.

	for (link = link_hd; link; link = link->next)
	{
// Is this link already established?
		if (link->flags & (p_linked | p_linkini)) continue;
// Already linked through some other node?
// If so, making this link would create a loop.
		if (node_find(link->call)) continue;
// Fire up the process to handle this link.
		link->flags = p_linkini;
		pcreate("RT", RT_STACK, link_out, PR_LINK, link);
	}
}

// Log out the local user.

static void logout(CIRCUIT *circuit)
{
	USER *user;

	circuit->flags = p_nil;
	user = circuit->u.user;
	if (log_rt) tlogp("RT Logout");
	user_tell(user, id_leave);
	text_tellu(user, rtleave, NULL, o_all);
	cn_dec(circuit, user->node);
	user_leave(user);
	circuit_free(circuit);
}

// /K Command: Show known nodes.

static void show_nodes(void)
{
	NODE *node;

	tputs("Known Nodes:\n");

	for (node = node_hd; node; node = node->next) if (node->refcnt)
		tprintf("%s %u\n", node->alias, node->refcnt);
}

// /P Command: List circuits and remote RT on them.

#define xxx "\n        "

static void show_circuits(void)
{
	CIRCUIT *circuit;
	NODE    *node;
	int     len;

	tprintf("Here %-6.6s <- ", Node->aliass);
	len = 0;

	for (node = node_hd; node; node = node->next) if (node->refcnt)
	{
		len += strlen(node->alias) + 1;
		if (len > 60) { len = strlen(node->alias) + 1; tputs(xxx); }
		tputs(node->alias);
		tputc(' ');
	}

	tputc('\n');

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	if (circuit->flags & p_linked)
	{
		tprintf("Link %-6.6s <- ", circuit->u.link->alias);
		len = 0;

		for (node = node_hd; node; node = node->next)
		if (node->refcnt && !cn_find(circuit, node))
		{
			len += strlen(node->alias) + 1;
			if (len > 60) { len = strlen(node->alias) + 1; tputs(xxx); }
			tputs(node->alias);
			tputc(' ');
		}

		tputc('\n');
	}
	else if (circuit->flags & p_user)
		tprintf("User %-6.6s\n", circuit->u.user->call);
	else if (circuit->flags & p_linkini)
		tprintf("Link %-6.6s (setup)\n", circuit->u.link->alias);
}

// /T Command: List topics and users in them.

static void show_topics(void)
{
	TOPIC *topic;
	USER  *user;

	tputs("Topics in the RoundTable are:\n");

	for (topic = topic_hd; topic; topic = topic->next)
	{
		tprintf("%s\n", topic->name);

		if (topic->refcnt)
		{
			tputs("  ");
			for (user = user_hd; user; user = user->next)
				if (user->topic is topic) tprintf(" %s", user->call);
			tputc('\n');
		}
	}
}

// /U Command: List users in the RoundTable.

static void show_users(void)
{
	USER *user;

	tputs("Stations in the RoundTable:\n");

	for (user = user_hd; user; user = user->next)
		tprintf("%-6.6s at %-9.9s %s, %s [ %s ]\n",
		user->call, user->node->alias, user->name, user->qth, user->topic->name);
}

// Do a user command.

static int rt_cmd(CIRCUIT *circuit)
{
	CIRCUIT *c;
	USER    *user, *su;
	char    *f1, *f2;

	user = circuit->u.user;

	switch(tolower(circuit->buf[1]))
	{
		case 'b' :                               return FALSE;
		case 'e' : user->flags flipbit u_echo;   return true;
		
		case 'h' :
			tputs("/U - Show Users.\n/N - Enter your Name.\n/Q - Enter your QTH.\n/T - Show Topics.\n");
			tputs("/T Name - Join Topic or Create new Topic.\n/P - Show Ports and Links.\n");
			tputs("/E - Toggle Echo.\n/S CALL Text - Send Text to that station only.\n");
			tputs("/F - Force all links to be made.\n/K - Show Known nodes.\n");
			tputs("/B - Leave RoundTable and return to node.\n");
			return true;
		
		case 'k' : show_nodes();                 return true;

		case 'n' :
			strnew(&user->name, circuit->buf + 3);
			saydone();
			upduser(user);
			user_tell(user, id_user);
			return true;

		case 'p' : show_circuits(); return true;

		case 'q' :
			strnew(&user->qth, circuit->buf + 3);
			saydone();
			upduser(user);
			user_tell(user, id_user);
			return true;

		case 's' :
			strcat(circuit->buf, "\n");
			f1 = strlop(circuit->buf, ' ');  // To.
			if (!f1) break;
			f2 = strlop(f1, ' ');            // Text to send.
			if (!f2) break;
			strupr(f1);
			su = user_find(f1);

			if (!su)
			{
				tputs("*** That user is not logged in.\n");
				return true;
			}

// Send to the desired user only.

			if (su->circuit->flags & p_user)
				text_tellu(user, f2, f1, o_one);
			else
				text_xmit(user, su, f2);

			return true;

		case 't' :
			f1 = strlop(circuit->buf, ' ');
			if (f1)
			{
				topic_chg(user, f1);

// Tell all link circuits about the change of topic.

				for (c = circuit_hd; c; c = c->next)
					if (c->flags & p_linked) topic_xmit(user, c);
			}
			else
			  show_topics();
			return true;

		case 'u' : show_users(); return true;

		default  : break;
	}

	saywhat();
	return true;
}

// Handle an incoming link.

int rtloginl ()
{
	LINK    *link;
	CIRCUIT *circuit;
	SADDR   sp;
	char    *buf, call[ln_axaddr+1];

	if (MemLow) return cmd_exit;
	getpeername(CurProc->input, &sp);
	if (sp.type isnt TYPE_NETROM) return cmd_exit;
	ax2str(call, sp.a.nr.node);
	if (node_find(call)) return cmd_exit; // Already linked.

	for (link = link_hd; link; link = link->next)
		if (matchi(call, link->call)) break;

	if (!link) return cmd_exit;           // We don't link with this system.
	if (link->flags & (p_linked | p_linkini)) return cmd_exit;  // Already linked.

// Accept the link request.

	puser(link->alias);
	buf = mallocw(LINE128);
	strcpy(buf, call);
	strlop(buf, '-');
	puser(buf);
	free(buf);

// Create a circuit for this link.

	circuit = circuit_new(p_linked, CurProc->output);
	if (!circuit) return cmd_exit;

	tputs("OK\n");
	circuit->u.link = link;
	link->flags     = p_linked;
	state_tell(circuit);
	makelinks();

// Run in circles, scream and shout.

	for (;;) if (getinp(circuit))
		chkctl(circuit);
	else
	{
		link_drop(circuit);
		link->flags = p_nil;
		return cmd_exit;
	}
}

// User connected to the node did the RT command.

int rtloginu ()
{
	CIRCUIT *c, *circuit;
	USER    *user;

// Is this user already logged in to RT somewhere else?

	if (user_find(CurProc->user))
	{
		tputs("*** Already connected at another node.\n");
		return cmd_exit;
	}

	if (log_rt) tlogp("RT Login");

// Create a circuit for this user.

	circuit = circuit_new(p_user, CurProc->output);
	if (!circuit) return cmd_exit;

// Create the user entry.

	user = user_join(circuit, CurProc->user, Node->calls, Node->aliass);
	circuit->u.user = user;
	tputs("RoundTable Server.\nType /h for command summary.\nBringing up links to other nodes.\n");
	tputs("This may take a minute or two.\nThe /p command shows what nodes are linked.\n");
	text_tellu(user, rtjoin, NULL, o_all);
	user_tell(user, id_join);
	show_users();
	makelinks();

// Run in circles, scream and shout.

	for (;;) if (getinp(circuit))
	{
		if (circuit->buf[0] is '/')
		{
			if (!rt_cmd(circuit))
			{
				tputs("Returned to node.\n");
				logout(circuit);
				return cmd_ok;
			}
		}
		else
		{
			text_tellu(user, circuit->buf, NULL, o_topic); // To local users.

// To remote users.

			for (c = circuit_hd; c; c = c->next)
			if ((c->flags & p_linked) && c->refcnt && ct_find(c, user->topic))
				nprintf(c->s, "%c%c%s %s %s\n",
				FORMAT, id_data, Node->calls, user->call, circuit->buf);
		}
	}
	else
	{
		logout(circuit);
		return cmd_exit;
	}
}

// Kill off all the RT processes.

static int rtkill ()
{
	rtrun = FALSE;
	return cmd_ok;
}

static int rtlink ()
{
	LINK *link;
	char *c;

	strupr(argv[1]);
	c = strlop(argv[1], ':');
	if (!c) return cmd_bad;

	link = zallocw(sizeof(LINK));
	sl_ins_hd(link, link_hd);
	link->alias = strdup(argv[1]);
	link->call  = strdup(c);
	return cmd_ok;
}

static int rtstat ()
{
	LINK    *link;
	NODE    *node;
	CN      *cn;
	CIRCUIT *circuit;
	TOPIC   *topic;
	USER    *user;

	for (link = link_hd; link; link = link->next)
		tprintf("Link %s:%s %x\n", link->alias, link->call, link->flags);

	for (user = user_hd; user; user = user->next)
		tprintf("User %s @ %s\n", user->call, user->node->alias);

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
		if (circuit->flags & p_linked)
	{
		tprintf("From %s:", circuit->u.link->alias);
		for (cn = circuit->hnode; cn; cn = cn->next)
	  	tprintf(" %s/%u", cn->node->alias, cn->refcnt);
		tputc('\n');
	}

	for (node = node_hd;  node; node = node->next)
		tprintf("Node %s %u\n", node->alias, node->refcnt);

	for (topic = topic_hd; topic; topic = topic->next)
		tprintf("Topic %s %u\n", topic->name, topic->refcnt);

	return cmd_ok;
}

static CMDDEF rtcmdl[] =
{
	"kill",    rtkill, 0, 1, cmd_cron,
	"link",    rtlink, 0, 2, cmd_cron,
	"status",  rtstat, 0, 1, cmd_cron,
	NULL,
};

CMDLIST Rtcmds = { &Topcmds, "rt", rtcmdl };
