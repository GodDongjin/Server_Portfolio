using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class InputText : MonoBehaviour
{
    public InputField inputField;
    public Text loginText;

	//public string inputString { get { return inputString; } set { inputString = value; } }

    void Start()
    {
		inputField.onEndEdit.AddListener(OnInputFieldEndEdit);
    }

    void OnInputFieldEndEdit(string text)
    {
		loginText.text = text.ToString();
	}
}
