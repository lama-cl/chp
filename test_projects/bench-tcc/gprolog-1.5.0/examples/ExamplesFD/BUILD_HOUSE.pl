/*
 * This is a basic problem that involves building a house. The masonry,
 * roofing, painting, etc.  must be scheduled. Some tasks must
 * necessarily take place before others, and these requirements are
 * expressed through precedence constraints.
 *
 *   Task	Description	Duration	Predecessors
 *     A	masonry	7	none
 *     B	carpentry	3	masonry
 *     C	roofing	1	carpentry
 *     D	plumbing	8	masonry
 *     E	facade	2	roofing, plumbing
 *     F	windows	1	roofing
 *     G	garden	1	roofing, plumbing
 *     H	ceiling	3	masonry
 *     I	painting	2	ceiling
 *     J	moving	1	windows, facade, garden, painting
 *     
 * sol: [0,7,7,7,10,10,11,15,15,17]
 * or (for duration * 5)
 * sol: [0,35,35,35,50,50,55,75,75,85]
 */

p(L) :-
    tasks(Ts),
    tell_cstr(Ts, Ts, L),
    last(L, End),
    fd_domain(L, 0, 1000),
    %fd_labeling(End).
    fd_minimize(fd_labeling(L), End).



tell_cstr([], _, []).

tell_cstr([t(X, NameX, _Desc, _Dur, Preds)|RestTs], Ts, [X|L]) :-
    cstr_for_preds(Preds, X, NameX, Ts),
    tell_cstr(RestTs, Ts, L).

cstr_for_preds([], _, _, _).

cstr_for_preds([Desc|Preds], X, NameX, Ts) :-
    member(t(Y, NameY, Desc, DurY, _), Ts), !,
    format('~w + ~w <= ~w~n', [NameY, DurY, NameX]),
    Y + DurY #=< X,
    cstr_for_preds(Preds, X, NameX, Ts).


disp_tasks :-
    tasks(Ts),
    disp_tasks(Ts, Ts).

disp_tasks([], _).

disp_tasks([t(_X, NameX, Desc, Dur, Preds)|RestTs], Ts) :-
    format('~a ~w ~d', [NameX, Desc, Dur]),
    disp_preds(Preds, Ts),
    nl,
    disp_tasks(RestTs, Ts).

disp_preds([], _).

disp_preds([Desc|Preds], Ts) :-
    member(t(_, NameY, Desc, _, _), Ts), !,
    format(' ~a', [NameY]),
    disp_preds(Preds, Ts).

/*
tasks([
	     t(_, 'A', masonry, 7, []),
	     t(_, 'B', carpentry, 3, [masonry]),
	     t(_, 'C', plumbing, 8, [masonry]),
	     t(_, 'D', ceiling, 3, [masonry]),
	     t(_, 'E', roofing, 1, [carpentry]),
	     t(_, 'F', painting, 2, [ceiling]),
	     t(_, 'G', windows, 1, [roofing]),
	     t(_, 'H', facade, 2, [roofing, plumbing]),
	     t(_, 'I', garden, 1, [roofing, plumbing]),
	     t(_, 'J', moving, 1, [windows, facade, garden, painting])]).
*/
% here duration are multiplied by 5
tasks([
	     t(_, 'A', masonry, 35, []),
	     t(_, 'B', carpentry, 15, [masonry]),
	     t(_, 'C', plumbing, 40, [masonry]),
	     t(_, 'D', ceiling, 15, [masonry]),
	     t(_, 'E', roofing, 5, [carpentry]),
	     t(_, 'F', painting, 10, [ceiling]),
	     t(_, 'G', windows, 5, [roofing]),
	     t(_, 'H', facade, 10, [roofing, plumbing]),
	     t(_, 'I', garden, 5, [roofing, plumbing]),
	     t(_, 'J', moving, 5, [windows, facade, garden, painting])]).




