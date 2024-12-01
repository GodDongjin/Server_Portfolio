using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class TextManager : MonoBehaviour
{
	private Text nameText;
	private Text lvText;
	private Text hpText;
	private Text atkText;
	private Text dfText;

	private Text createNameText;
	private Text dupliCationText;

	private InputField inputField;


	public Text GetCreateNameText() { return createNameText; }
	

	public void Seting()
	{
		nameText = GameObject.Find("name_Text").GetComponent<Text>();
		lvText = GameObject.Find("lv_Text").GetComponent<Text>();
		hpText = GameObject.Find("hp_Text").GetComponent<Text>();
		atkText = GameObject.Find("ATK_Text").GetComponent<Text>();
		dfText = GameObject.Find("DF_Text").GetComponent<Text>();

		createNameText = GameObject.Find("CreateName_Text").GetComponent<Text>();
		dupliCationText = GameObject.Find("DupliCation_Text").GetComponent<Text>();

		inputField = GameObject.Find("NameBar").GetComponent<InputField>();
		inputField.characterLimit = 10;
	}

	public void SetPlayerInfo(Protocol.PlayerInfo playerInfo)
	{
		GameManager.Instance.Enqueue(() =>
		{
			nameText.text += (" " + playerInfo.PlayerName.ToString());
			lvText.text += (" " + playerInfo.Lv.ToString());
			hpText.text += (" " + playerInfo.Hp.ToString());
			atkText.text += (" " + playerInfo.Atk.ToString());
			dfText.text += (" " + playerInfo.Df.ToString());
		});
	}

	public void OnDuplication()
	{
		GameManager.Instance.Enqueue(() =>
		{
			dupliCationText.text = "중복된 이름이 있습니다.";
		});
	}
}
